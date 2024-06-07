#include "add_symbol.h"
#include "debug.h"
#include "re.h"
#include "span.h"
#include <assert.h>
#include <clink/asm.h>
#include <clink/db.h>
#include <clink/symbol.h>
#include <errno.h>
#include <fcntl.h>
#include <regex.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// regex for recognising a #define
static const char DEFINE[] =
    "^[[:blank:]]*#[[:blank:]]*define[[:blank:]]+([[:alpha:]_][[:alnum:]_]*)";

// regex for recognising a #include
static const char INCLUDE[] =
    "^[[:blank:]]*#[[:blank:]]*include[[:blank:]]*(<[^>]*>|\"[^\"]*\")";

// regex for recognising a function definition
static const char FUNCTION[] =
    "^[[:blank:]]*([[:alpha:]\\._][[:alnum:]\\._\\$@]*)[[:blank:]]*:";

// regex for recognising a branch
static const char CALL[] =
    "^[[:blank:]]*("

    // ARM. Note, we omit some jumps like bx that take a register, cbz that
    // are rarely used for long jumps and literal pool loads that are more
    // complex to parse.
    "b|beq|bne|bcs|bhs|bcc|blo|bmi|bpl|bvs|bvc|bhi|bls|bge|blt|bgt|ble|bal|bl"
    "|bleq|blne|blcs|blhs|blcc|bllo|blmi|blpl|blvs|blvc|blhi|blls|blge|bllt"
    "|blgt|blle|blal|blx|blxeq|blxne|blxcs|blxhs|blxcc|blxlo|blxmi|blxpl|blxvs"
    "|blxvc|blxhi|blxls|blxge|blxlt|blxgt|blxle|blxal"

    // AVR. We omit brb{c|s} that take a bit index as the first parameter.
    "|brcc|brcs|breq|brge|brhc|brhs|brid|brie|brlo|brlt|brmi|brme|brpl|brsh"
    "|brtc|brts|brvc|brvs|jmp"

    // MIPS. Support here isn't great because MIPS has a set of instructions
    // that take registers to compare as the first parameters (beq and
    // friends). Parsing these requires more acrobatics than we're willing
    // to admit.
    "|j|jal"

    // PowerPC. Note that we omit bne which is a bit trickier to parse.
    "|b|ba|bl|bla|blt|bdnz"

    // RISC-V. Note we omit all the conditional branch instructions.
    "|jal"

    // x86. Note that we omit some jumps like loop that are rarely used for
    // function calls.
    "|call|callq|ja|jae|jb|jbe|jc|jcxz|je|jecxz|jg|jge|jl|jle|jmp|jna|jnae|jnb"
    "|jnbe|jnc|jne|jng|jnge|jnl|jnle|jno|jnp|jns|jnz|jo|jp|jpe|jpo|js|jz"

    ")[[:blank:]]+([[:alpha:]\\._][[:alnum:]\\._\\$@]*)";

// state used by an ASM parsing iterator
typedef struct {

  /// database we are inserting into
  clink_db_t *db;

  /// filename of the source
  const char *filename;

  /// source file being read
  FILE *in;

  /// line number in the source file we have read up to
  unsigned long lineno;

  /// regex structure for recognising a #define
  regex_t define;
  bool define_valid;

  /// regex structure for recognising a #include
  regex_t include;
  bool include_valid;

  /// regex structure for recognising a function definition
  regex_t function;
  bool function_valid;

  /// regex structure for recognising a branch
  regex_t call;
  bool call_valid;

} state_t;

/// add a symbol to our pending next-to-yield slot
static int add(const state_t *s, clink_category_t cat, const char *line,
               const regmatch_t *m, const char *parent) {

  assert(s != NULL);
  assert(line != NULL);
  assert(m != NULL);

  span_t name = {.base = line + m->rm_so,
                 .size = m->rm_eo - m->rm_so,
                 .lineno = s->lineno,
                 .colno = m->rm_so + 1};
  span_t par = {0};
  if (parent != NULL)
    par = (span_t){.base = parent, .size = strlen(parent)};

  symbol_t sym = {
      .category = cat, .name = name, .path = s->filename, .parent = par};

  return add_symbol(s->db, sym);
}

/// run the assembly parsing job described by our state parameter
static int parse(state_t *s) {

  assert(s != NULL);

  int rc = 0;

  // optional function that we are currently within during parsing
  char *parent = NULL;

  char *line = NULL;
  size_t line_size = 0;
  while (true) {

    // update position in file
    ++s->lineno;

    errno = 0;
    if (getline(&line, &line_size, s->in) < 0) {
      rc = errno;
      break;
    }

    // is this a #define?
    {
      regmatch_t m[2];
      size_t m_len = sizeof(m) / sizeof(m[0]);
      int r = regexec(&s->define, line, m_len, m, 0);
      if (r == 0) { // match
        rc = add(s, CLINK_DEFINITION, line, &m[1], NULL);
      } else if (ERROR(r != REG_NOMATCH)) { // error
        rc = re_err_to_errno(r);
      }
      if (rc != 0)
        break;
    }

    // is this a #include?
    {
      regmatch_t m[2];
      size_t m_len = sizeof(m) / sizeof(m[0]);
      int r = regexec(&s->include, line, m_len, m, 0);
      if (r == 0) { // match

        // adjust to chop delimiters
        ++m[1].rm_so;
        --m[1].rm_eo;

        rc = add(s, CLINK_INCLUDE, line, &m[1], NULL);
      } else if (ERROR(r != REG_NOMATCH)) { // error
        rc = re_err_to_errno(r);
      }
      if (rc != 0)
        break;
    }

    // is this a function?
    {
      regmatch_t m[2];
      size_t m_len = sizeof(m) / sizeof(m[0]);
      int r = regexec(&s->function, line, m_len, m, 0);
      if (r == 0) { // match
        if (ERROR((rc = add(s, CLINK_DEFINITION, line, &m[1], NULL))))
          break;

        // save the context we  are now assumed to be within
        free(parent);
        parent = strndup(line + m[1].rm_so, m[1].rm_eo - m[1].rm_so);
        if (ERROR(parent == NULL))
          rc = ENOMEM;

      } else if (ERROR(r != REG_NOMATCH)) { // error
        rc = re_err_to_errno(r);
      }
      if (rc != 0)
        break;
    }

    // is this a branch?
    {
      regmatch_t m[3];
      size_t m_len = sizeof(m) / sizeof(m[0]);
      int r = regexec(&s->call, line, m_len, m, 0);
      if (r == 0) { // match
        rc = add(s, CLINK_FUNCTION_CALL, line, &m[2], parent);
      } else if (ERROR(r != REG_NOMATCH)) { // error
        rc = re_err_to_errno(r);
      }
      if (rc != 0)
        break;
    }
  }

  free(line);
  free(parent);

  return rc;
}

static void state_free(state_t *s) {

  if (s->call_valid)
    regfree(&s->call);
  s->call_valid = false;

  if (s->function_valid)
    regfree(&s->function);
  s->function_valid = false;

  if (s->include_valid)
    regfree(&s->include);
  s->include_valid = false;

  if (s->define_valid)
    regfree(&s->define);
  s->define_valid = false;

  if (s->in != NULL)
    (void)fclose(s->in);
  s->in = NULL;
}

/// `fopen` with "r" that also sets close-on-exec
static FILE *fopen_r(const char *path) {
  assert(path != NULL);

#ifdef __APPLE__
  // macOS does not support 'e' to `fopen`, so work around this
  const int fd = open(path, O_RDONLY | O_CLOEXEC);
  if (fd < 0)
    return NULL;

  FILE *f = fdopen(fd, "r");
  if (f == NULL) {
    const int err = errno;
    (void)close(fd);
    errno = err;
  }

  return f;
#else
  return fopen(path, "re");
#endif
}

int clink_parse_asm(clink_db_t *db, const char *filename) {

  if (ERROR(db == NULL))
    return EINVAL;

  if (ERROR(filename == NULL))
    return EINVAL;

  if (ERROR(filename[0] != '/'))
    return EINVAL;

  int rc = 0;
  state_t s = {.db = db, .filename = filename};

  // open the input file
  s.in = fopen_r(filename);
  if (s.in == NULL) {
    rc = errno;
    goto done;
  }

  // construct regex for recognising a #define
  if (ERROR((rc = regcomp(&s.define, DEFINE, REG_EXTENDED)))) {
    rc = re_err_to_errno(rc);
    goto done;
  }
  s.define_valid = true;

  // construct regex for recognising a #include
  if (ERROR((rc = regcomp(&s.include, INCLUDE, REG_EXTENDED)))) {
    rc = re_err_to_errno(rc);
    goto done;
  }
  s.include_valid = true;

  // construct regex for recognising a function definition
  if (ERROR((rc = regcomp(&s.function, FUNCTION, REG_EXTENDED)))) {
    rc = re_err_to_errno(rc);
    goto done;
  }
  s.function_valid = true;

  // construct regex for recognising a branch
  if (ERROR((rc = regcomp(&s.call, CALL, REG_EXTENDED)))) {
    rc = re_err_to_errno(rc);
    goto done;
  }
  s.call_valid = true;

  rc = parse(&s);

done:
  state_free(&s);

  return rc;
}
