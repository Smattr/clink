#include <assert.h>
#include <clink/asm.h>
#include <clink/iter.h>
#include <clink/symbol.h>
#include <errno.h>
#include "iter.h"
#include "re.h"
#include <regex.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

  /// filename of the source
  char *filename;

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

  /// optional function that we are currently within during parsing
  char *parent;

  /// last yielded symbol
  clink_symbol_t last;

} state_t;

/// add a symbol to our pending next-to-yield slot
static int add_symbol(state_t *s, clink_category_t cat, const char *line,
    const regmatch_t *m, const char *parent) {

  assert(s != NULL);
  assert(line != NULL);
  assert(m != NULL);

  int rc = 0;

  s->last.category = cat;

  s->last.name = strndup(line + m->rm_so, m->rm_eo - m->rm_so);
  if (s->last.name == NULL) {
    rc = ENOMEM;
    goto done;
  }

  s->last.path = strdup(s->filename);
  if (s->last.path == NULL) {
    rc = ENOMEM;
    goto done;
  }

  s->last.lineno = s->lineno;

  s->last.colno = m->rm_so + 1;

  if (parent != NULL) {
    s->last.parent = strdup(parent);
    if (s->last.parent == NULL) {
      rc = ENOMEM;
      goto done;
    }
  }

done:
  if (rc)
    clink_symbol_clear(&s->last);

  return rc;
}

/// populate s->last with the next-to-yield symbol
static int refill_last(state_t *s) {

  assert(s != NULL);

  // discard the previous if necessary
  clink_symbol_clear(&s->last);

  // now we want to find at least one new symbol

  int rc = 0;

  char *line = NULL;
  size_t line_size = 0;
  for (;;) {

    // update position in file
    ++s->lineno;

    errno = 0;
    if (getline(&line, &line_size, s->in) < 0) {
      if (errno) {
        rc = errno;
      } else { // EOF
        rc = ENOMSG;
      }
      break;
    }

    // is this a #define?
    {
      regmatch_t m[2];
      size_t m_len = sizeof(m) / sizeof(m[0]);
      int r = regexec(&s->define, line, m_len, m, 0);
      if (r == 0) { // match
        rc = add_symbol(s, CLINK_DEFINITION, line, &m[1], NULL);
        break;
      } else if (r != REG_NOMATCH) { // error
        rc = re_err_to_errno(r);
        break;
      }
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

        rc = add_symbol(s, CLINK_INCLUDE, line, &m[1], NULL);
        break;
      } else if (r != REG_NOMATCH) { // error
        rc = re_err_to_errno(r);
        break;
      }
    }

    // is this a function?
    {
      regmatch_t m[2];
      size_t m_len = sizeof(m) / sizeof(m[0]);
      int r = regexec(&s->function, line, m_len, m, 0);
      if (r == 0) { // match
        if ((rc = add_symbol(s, CLINK_DEFINITION, line, &m[1], NULL)))
          break;

        // save the context we  are now assumed to be within
        free(s->parent);
        s->parent = strndup(line + m[1].rm_so, m[1].rm_eo - m[1].rm_so);
        if (s->parent == NULL)
          rc = ENOMEM;

        break;
      } else if (r != REG_NOMATCH) { // error
        rc = re_err_to_errno(r);
        break;
      }
    }

    // is this a branch?
    {
      regmatch_t m[3];
      size_t m_len = sizeof(m) / sizeof(m[0]);
      int r = regexec(&s->call, line, m_len, m, 0);
      if (r == 0) { // match
        rc = add_symbol(s, CLINK_FUNCTION_CALL, line, &m[2], s->parent);
        break;
      } else if (r != REG_NOMATCH) { // error
        rc = re_err_to_errno(r);
        break;
      }
    }
  }

  free(line);

  return rc;
}

static int next(no_lookahead_iter_t *it, const clink_symbol_t **yielded) {

  if (it == NULL)
    return EINVAL;

  if (yielded == NULL)
    return EINVAL;

  state_t *s = it->state;
  if (s == NULL)
    return EINVAL;

  // acquire a new next symbol
  int rc = refill_last(s);
  if (rc)
    return rc;

  // yield this
  *yielded = &s->last;
  return 0;
}

static void my_free(no_lookahead_iter_t *it) {

  if (it == NULL)
    return;

  state_t *s = it->state;
  if (s == NULL)
    return;

  clink_symbol_clear(&s->last);

  free(s->parent);
  s->parent = NULL;

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

  free(s->filename);
  s->filename = NULL;

  free(s);
  it->state = NULL;
}

int clink_parse_asm(clink_iter_t **it, const char *filename) {

  if (it == NULL)
    return EINVAL;

  if (filename == NULL)
    return EINVAL;

  clink_iter_t *wrapper = NULL;

  // allocate a new no-lookahead iterator
  no_lookahead_iter_t *i = calloc(1, sizeof(*i));
  if (i == NULL)
    return ENOMEM;

  // setup our member functions
  i->next_symbol = next;
  i->free = my_free;

  int rc = 0;

  // allocate state for our iterator
  state_t *s = calloc(1, sizeof(*s));
  if (s == NULL) {
    rc = ENOMEM;
    goto done;
  }
  i->state = s;

  // save the filename for later use in symbol construction
  s->filename = strdup(filename);
  if (s->filename == NULL) {
    rc = ENOMEM;
    goto done;
  }

  // open the input file
  s->in = fopen(filename, "r");
  if (s->in == NULL) {
    rc = errno;
    goto done;
  }

  // construct regex for recognising a #define
  if ((rc = regcomp(&s->define, DEFINE, REG_EXTENDED))) {
    rc = re_err_to_errno(rc);
    goto done;
  }
  s->define_valid = true;

  // construct regex for recognising a #include
  if ((rc = regcomp(&s->include, INCLUDE, REG_EXTENDED))) {
    rc = re_err_to_errno(rc);
    goto done;
  }
  s->include_valid = true;

  // construct regex for recognising a function definition
  if ((rc = regcomp(&s->function, FUNCTION, REG_EXTENDED))) {
    rc = re_err_to_errno(rc);
    goto done;
  }
  s->function_valid = true;

  // construct regex for recognising a branch
  if ((rc = regcomp(&s->call, CALL, REG_EXTENDED))) {
    rc = re_err_to_errno(rc);
    goto done;
  }
  s->call_valid = true;

  // create a 1-lookahead adapter to wrap this
  if ((rc = iter_new(&wrapper, i)))
    goto done;

done:
  if (rc) {
    no_lookahead_iter_free(&i);
    clink_iter_free(&wrapper);
  } else {
    *it = wrapper;
  }

  return rc;
}
