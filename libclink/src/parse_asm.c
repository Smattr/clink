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

  /// possibly valid next symbols to yield
  clink_symbol_t next[2];
  bool next_valid[2];

  /// whether we previously yielded next[0]
  bool head_yielded;

} state_t;

static bool has_next(const clink_iter_t *it) {

  if (it == NULL)
    return false;

  const state_t *s = it->state;

  if (s == NULL)
    return false;

  // do we have a valid next symbol to yield?
  return (!s->head_yielded && s->next_valid[0]) || s->next_valid[1];
}

/// add a symbol to our pending next-to-yield list
static int add_symbol(state_t *s, clink_category_t cat, const char *line,
    const regmatch_t *m, const char *parent) {

  assert(s != NULL);
  assert(!s->next_valid[1]);
  assert(line != NULL);
  assert(m != NULL);

  // which slot are we adding into?
  clink_symbol_t *sym = s->next_valid[0] ? &s->next[1] : &s->next[0];
  bool *valid         = s->next_valid[0] ? &s->next_valid[1] : &s->next_valid[0];

  assert(!*valid);

  int rc = 0;

  sym->category = cat;

  sym->name = strndup(line + m->rm_so, m->rm_eo - m->rm_so);
  if (sym->name == NULL) {
    rc = ENOMEM;
    goto done;
  }

  sym->path = strdup(s->filename);
  if (sym->path == NULL) {
    rc = ENOMEM;
    goto done;
  }

  sym->lineno = s->lineno;

  sym->colno = m->rm_so + 1;

  if (parent != NULL) {
    sym->parent = strdup(parent);
    if (sym->parent == NULL) {
      rc = ENOMEM;
      goto done;
    }
  }

  *valid = true;

done:
  if (rc)
    clink_symbol_clear(sym);

  return rc;
}

static int move_next(state_t *s) {

  assert(s != NULL);

  // discard the head if necessary
  if (s->next_valid[0]) {
    clink_symbol_clear(&s->next[0]);
    s->next_valid[0] = false;
    s->head_yielded = false;
  }

  // shuffle the queue forwards
  s->next[0] = s->next[1];
  memset(&s->next[1], 0, sizeof(s->next[1]));
  s->next_valid[0] = s->next_valid[1];
  s->next_valid[1] = false;

  // now we want to find at least one new symbol

  int rc = 0;

  char *line = NULL;
  size_t line_size = 0;
  while (!s->next_valid[1]) {

    // update position in file
    ++s->lineno;

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
        if ((rc = add_symbol(s, CLINK_DEFINITION, line, &m[1], NULL)))
          goto done;
        continue;
      } else if (r != REG_NOMATCH) { // error
        rc = re_err_to_errno(r);
        goto done;
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

        if ((rc = add_symbol(s, CLINK_INCLUDE, line, &m[1], NULL)))
          goto done;
        continue;
      } else if (r != REG_NOMATCH) { // error
        rc = re_err_to_errno(r);
        goto done;
      }
    }

    // is this a function?
    {
      regmatch_t m[2];
      size_t m_len = sizeof(m) / sizeof(m[0]);
      int r = regexec(&s->function, line, m_len, m, 0);
      if (r == 0) { // match
        if ((rc = add_symbol(s, CLINK_DEFINITION, line, &m[1], NULL)))
          goto done;

        // save the context we  are now assumed to be within
        free(s->parent);
        s->parent = strndup(line + m[1].rm_so, m[1].rm_eo - m[1].rm_so);
        if (s->parent == NULL) {
          rc = ENOMEM;
          goto done;
        }

        continue;
      } else if (r != REG_NOMATCH) { // error
        rc = re_err_to_errno(r);
        goto done;
      }
    }

    // is this a branch?
    {
      regmatch_t m[3];
      size_t m_len = sizeof(m) / sizeof(m[0]);
      int r = regexec(&s->call, line, m_len, m, 0);
      if (r == 0) { // match
        if ((rc = add_symbol(s, CLINK_FUNCTION_CALL, line, &m[2], s->parent)))
          goto done;
        continue;
      } else if (r != REG_NOMATCH) { // error
        rc = re_err_to_errno(r);
        goto done;
      }
    }
  }

done:
  free(line);

  return rc;
}

static int next(clink_iter_t *it, const clink_symbol_t **yielded) {

  if (it == NULL)
    return EINVAL;

  if (yielded == NULL)
    return EINVAL;

  state_t *s = it->state;
  if (s == NULL)
    return EINVAL;

  if (!has_next(it))
    return EINVAL;

  // if we previously yielded the head, we need to shuffle the pending elements
  // forwards
  if (s->head_yielded) {

    assert(s->next_valid[0]);

    int rc = move_next(s);
    if (rc)
      return rc;
  }

  // now we should have an unyielded head
  assert(s->next_valid[0] && !s->head_yielded);

  *yielded = &s->next[0];
  s->head_yielded = true;

  return 0;
}

static void my_free(clink_iter_t *it) {

  if (it == NULL)
    return;

  state_t *s = it->state;
  if (s == NULL)
    return;

  for (size_t i = 0; i < sizeof(s->next) / sizeof(s->next[0]); ++i) {
    clink_symbol_clear(&s->next[i]);
    s->next_valid[i] = false;
  }
  s->head_yielded = false;

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

  // allocate a new iterator
  clink_iter_t *i = calloc(1, sizeof(*i));
  if (i == NULL)
    return ENOMEM;

  // setup our member functions
  i->has_next = has_next;
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

  // advance the iterator to the first symbol(s)
  if ((rc = move_next(s)))
    goto done;

done:
  if (rc) {
    clink_iter_free(&i);
  } else {
    *it = i;
  }

  return rc;
}
