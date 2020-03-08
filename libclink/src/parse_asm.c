#include <assert.h>
#include <clink/parse_asm.h>
#include <clink/symbol.h>
#include <errno.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// regex for recognising a #define
static const char DEFINE[]
  = "^[[:blank:]]*#[[:blank:]]*define[[:blank:]]+([[:alpha:]_][[:alnum:]_]*)";

// regex for recognising a #include
static const char INCLUDE[]
  = "^[[:blank:]]*#[[:blank:]]*include[[:blank:]]*(<[^>]*>|\"[^\"]*\")";

// regex for recognising a function definition
static const char FUNCTION[]
  = "^[[:blank:]]*([[:alpha:]\\._][[:alnum:]\\._\\$@]*)[[:blank:]]*:";

// regex for recognising a branch
static const char CALL[]
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

int clink_parse_asm(
    const char *filename,
    int (*callback)(const struct clink_symbol *symbol, void *state),
    void *state) {

  assert(filename != NULL);
  assert(callback != NULL);

  int rc = 0;

  // open the input file
  FILE *f = fopen(filename, "r");
  if (f == NULL)
    return errno;

  // construct regexes
  regex_t define;
  rc = regcomp(&define, DEFINE, REG_EXTENDED);
  if (rc != 0)
    goto fail1;

  regex_t include;
  rc = regcomp(&include, INCLUDE, REG_EXTENDED);
  if (rc != 0)
    goto fail2;

  regex_t function;
  rc = regcomp(&function, FUNCTION, REG_EXTENDED);
  if (rc != 0)
    goto fail3;

  regex_t call;
  rc = regcomp(&call, CALL, REG_EXTENDED);
  if (rc != 0)
    goto fail4;

  // an ASM function we may be within;
  char *parent = NULL;

  // read through the file, line-by-line
  char *line = NULL;
  size_t line_size = 0;
  for (unsigned long lineno = 1; getline(&line, &line_size, f) >= 0; ++lineno) {

    // is this a #define?
    {
      regmatch_t m[2];
      size_t size = sizeof(m) / sizeof(m[0]);
      if (regexec(&define, line, size, m, 0) == 0) {
        const struct clink_symbol s = {
          .category = CLINK_DEFINITION,
          .name = line + m[1].rm_so,
          .name_len = m[1].rm_eo - m[1].rm_so,
          .path = (char*)filename,
          .lineno = lineno,
          .colno = m[1].rm_so + 1,
          .parent = parent,
        };
        rc = callback(&s, state);
        if (rc != 0)
          goto done;
        continue;
      }
    }

    // is this a #include?
    {
      regmatch_t m[2];
      size_t size = sizeof(m) / sizeof(m[0]);
      if (regexec(&include, line, size, m, 0) == 0) {

        // some +1/-1 adjustments to chop delimiters
        ++m[1].rm_so;
        --m[1].rm_eo;

        const struct clink_symbol s = {
          .category = CLINK_INCLUDE,
          .name = line + m[1].rm_so,
          .name_len = m[1].rm_eo - m[1].rm_so,
          .path = (char*)filename,
          .lineno = lineno,
          .colno = m[1].rm_so + 1,
          .parent = parent,
        };
        rc = callback(&s, state);
        if (rc != 0)
          goto done;
        continue;
      }
    }

    // is this a function?
    {
      regmatch_t m[2];
      size_t size = sizeof(m) / sizeof(m[0]);
      if (regexec(&function, line, size, m, 0) == 0) {
        const struct clink_symbol s = {
          .category = CLINK_DEFINITION,
          .name = line + m[1].rm_so,
          .name_len = m[1].rm_eo - m[1].rm_so,
          .path = (char*)filename,
          .lineno = lineno,
          .colno = m[1].rm_so + 1,
          .parent = parent,
        };
        rc = callback(&s, state);
        if (rc != 0)
          goto done;

        // save context we are now assumed to be within
        size_t start = m[1].rm_so;
        size_t extent = m[1].rm_eo - start;
        free(parent);
        parent = strndup(line + start, extent);
        if (parent == NULL) {
          rc = ENOMEM;
          goto done;
        }

        continue;
      }
    }

    // is this a branch?
    {
      regmatch_t m[3];
      size_t size = sizeof(m) / sizeof(m[0]);
      if (regexec(&call, line, size, m, 0) == 0) {
        const struct clink_symbol s = {
          .category = CLINK_FUNCTION_CALL,
          .name = line + m[1].rm_so,
          .name_len = m[1].rm_eo - m[1].rm_so,
          .path = (char*)filename,
          .lineno = lineno,
          .colno = m[1].rm_so + 1,
          .parent = parent,
        };
        rc = callback(&s, state);
        if (rc != 0)
          goto done;
        continue;
      }
    }
  }

done:
  free(line);
  free(parent);

  regfree(&call);
fail4:
  regfree(&function);
fail3:
  regfree(&include);
fail2:
  regfree(&define);
fail1:
  fclose(f);

  return rc;
}
