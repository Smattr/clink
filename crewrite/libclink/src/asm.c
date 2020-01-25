#include "asm.h"
#include <assert.h>
#include <clink/asm.h>
#include <clink/compiler.h>
#include <clink/symbol.h>
#include <errno.h>
#include <regex.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// has this module been initialised?
static bool inited = false;

// regex for recognising a #define
static const char define_re[]
  = "^[[:blank:]]*#[[:blank:]]*define[[:blank:]]+([[:alpha:]_][[:alnum:]_]*)";
static regex_t define;

// regex for recognising a #include
static const char include_re[]
  = "^[[:blank:]]*#[[:blank:]]*include[[:blank:]]*(<[^>]*>|\"[^\"]*\")";
static regex_t include;

// regex for recognising a function definition
static const char function_re[]
  = "^[[:blank:]]*([[:alpha:]\\._][[:alnum:]\\._\\$@]*)[[:blank:]]*:";
static regex_t function;

// regex for recognising a branch
static const char call_re[]
  = "^[[:blank:]]*("

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
static regex_t call;

int clink_init_asm() {

  int rc = 0;

  // build the regexs we need
  if ((rc = regcomp(&define, define_re, REG_EXTENDED)))
    return rc;
  if ((rc = regcomp(&include, include_re, REG_EXTENDED)))
    return rc;
  if ((rc = regcomp(&function, function_re, REG_EXTENDED)))
    return rc;
  if ((rc = regcomp(&call, call_re, REG_EXTENDED)))
    return rc;

  inited = true;

  return rc;
}

void clink_deinit_asm() {

  if (!inited)
    return;

  inited = false;

  regfree(&call);
  regfree(&function);
  regfree(&include);
  regfree(&define);
}

static bool match(const regex_t *re, regmatch_t *result, const char *text) {

  assert(re != NULL);
  assert(result != NULL);
  assert(text != NULL);

  // do we have a match in the given text?
  regmatch_t m[2];
  if (regexec(re, text, sizeof(m) / sizeof(m[0]), m, 0) == 0) {
    *result = m[1];
    return true;
  }

  return false;
}

static bool match_define(regmatch_t *result, const char *line) {
  assert(inited);
  return match(&define, result, line);
}

static bool match_include(regmatch_t *result, const char *line) {
  assert(inited);
  return match(&include, result, line);
}

static bool match_function(regmatch_t *result, const char *line) {
  assert(inited);
  return match(&function, result, line);
}

int clink_parse_asm(
    const char *filename,
    int (*callback)(void *state, const clink_symbol_t *symbol),
    void *state) {

  assert(filename != NULL);
  assert(callback != NULL);

  if (!inited)
    return -1;

  int rc = 0;
  FILE *f = NULL;
  char *line = NULL;
  size_t size = 0;
  char *last_defn = NULL;

  f = fopen(filename, "r");
  if (f == NULL) {
    rc = errno;
    goto done;
  }

  for (unsigned lineno = 1;; lineno++) {

    // get the next line in the file
    ssize_t r = getline(&line, &size, f);
    if (r == -1) {
      if ((rc = errno))
        goto done;
      break;
    }

    // storage for any regex matches
    regmatch_t result;

    // is this a #define?
    if (match_define(&result, line)) {
      const clink_symbol_t symbol = {
        .category = CLINK_ST_DEFINITION,
        .name = line + result.rm_so,
        .name_len = result.rm_eo - result.rm_so,
        .path = filename,
        .line = lineno,
        .column = result.rm_so,
        .parent = last_defn,
      };
      if ((rc = callback(state, &symbol)))
        goto done;
      continue;
    }

    // is this a #include?
    if (match_include(&result, line)) {
      // some +1/-1 adjustments to chop delimiters
      const clink_symbol_t symbol = {
        .category = CLINK_ST_INCLUDE,
        .name = line + result.rm_so + 1,
        .name_len = result.rm_eo - result.rm_so - 2,
        .path = filename,
        .line = lineno,
        .column = result.rm_so + 1,
        .parent = last_defn,
      };
      if ((rc = callback(state, &symbol)))
        goto done;
      continue;
    }

    // is this a function?
    if (match_function(&result, line)) {
      const clink_symbol_t symbol = {
        .category = CLINK_ST_DEFINITION,
        .name = line + result.rm_so,
        .name_len = result.rm_eo - result.rm_so,
        .path = filename,
        .line = lineno,
        .column = result.rm_so,
        .parent = last_defn,
      };
      if ((rc = callback(state, &symbol)))
        goto done;
      last_defn = strndup(line + result.rm_so, result.rm_eo - result.rm_so);
      if (last_defn == NULL) {
        rc = ENOMEM;
        goto done;
      }
      continue;
    }

    // is this a branch?
    regmatch_t b[3];
    if (regexec(&call, line, sizeof(b) / sizeof(b[0]), b, 0) == 0) {
      const clink_symbol_t symbol = {
        .category = CLINK_ST_CALL,
        .name = line + b[2].rm_so,
        .name_len = b[2].rm_eo - b[2].rm_so,
        .path = filename,
        .line = lineno,
        .column = b[2].rm_so,
        .parent = last_defn,
      };
      if ((rc = callback(state, &symbol)))
        goto done;
      continue;
    }
  }

done:
  free(last_defn);
  free(line);

  if (f != NULL)
    fclose(f);

  return rc;
}

static int print(CLINK_UNUSED void *state, const clink_symbol_t *symbol) {

  const char *category =
    symbol->category == CLINK_ST_DEFINITION ? "definition" :
    symbol->category == CLINK_ST_INCLUDE ? "include" :
    symbol->category == CLINK_ST_CALL ? "function call" : "";

  printf("%s:%lu:%lu: %s of %.*s\n", symbol->path, symbol->line, symbol->column,
    category, (int)symbol->name_len, symbol->name);

  return 0;
}

static int CLINK_UNUSED main_asm(int argc, char **argv) {

  int rc = 0;

  if (argc != 2) {
    fprintf(stderr, "usage: %s filename\n", argv[0]);
    return EXIT_FAILURE;
  }

  if ((rc = clink_init_asm())) {
    fprintf(stderr, "clink_init_asm failed: \n");
    return rc;
  }

  if ((rc = clink_parse_asm(argv[1], print, NULL)))
    fprintf(stderr, "parsing failed: %s\n", strerror(rc));

  return rc;
}

#ifdef MAIN_ASM
int main(int argc, char **argv) {
  return main_asm(argc, argv);
}
#endif
