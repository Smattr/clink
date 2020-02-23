#include <cstddef>
#include <clink/parse_asm.h>
#include <clink/Symbol.h>
#include <errno.h>
#include <fstream>
#include <iostream>
#include "Re.h"
#include <string>
#include <sys/stat.h>
#include <unistd.h>

namespace clink {

// regex for recognising a #define
static const Re<2, 1> define{
  "^[[:blank:]]*#[[:blank:]]*define[[:blank:]]+([[:alpha:]_][[:alnum:]_]*)"};

// regex for recognising a #include
static const Re<2, 1> include{
  "^[[:blank:]]*#[[:blank:]]*include[[:blank:]]*(<[^>]*>|\"[^\"]*\")"};

// regex for recognising a function definition
static const Re<2, 1> function{
  "^[[:blank:]]*([[:alpha:]\\._][[:alnum:]\\._\\$@]*)[[:blank:]]*:"};

// regex for recognising a branch
static const Re<3, 2> call{
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

  ")[[:blank:]]+([[:alpha:]\\._][[:alnum:]\\._\\$@]*)"};

static Symbol make_symbol(Symbol::Category category, const std::string &filename,
    const std::string &line, unsigned long lineno, Match match,
    const std::string &parent) {

  return Symbol{
    category,
    line.substr(match.start_offset, match.end_offset - match.start_offset),
    filename,
    lineno,
    static_cast<unsigned long>(match.start_offset),
    parent,
  };
}

int parse_asm(const std::string &filename, int(*callback)(const Symbol&)) {

  // check the file exists
  struct stat ignored;
  if (stat(filename.c_str(), &ignored) != 0)
    return errno;

  // check we can open it for reading
  if (access(filename.c_str(), R_OK) != 0)
    return errno;

  // open the input file
  std::ifstream in(filename);
  if (!in.is_open())
    throw Error("failed to open " + filename);

  // an ASM function we may be within
  std::string parent;

  // read through the file, line-by-line
  std::string line;
  for (unsigned long lineno = 1; std::getline(in, line); ++lineno) {

    // is this a #define?
    if (std::optional<Match> match = define.match(line)) {

      Symbol s
        = make_symbol(Symbol::DEFINITION, filename, line, lineno, *match, "");
      int rc = callback(s);
      if (rc != 0)
        return rc;

      continue;
    }

    // is this a #include?
    if (std::optional<Match> match = include.match(line)) {

      // some +1/-1 adjustments to chop delimiters
      ++match->start_offset;
      --match->end_offset;

      Symbol s
        = make_symbol(Symbol::INCLUDE, filename, line, lineno, *match, "");
      int rc = callback(s);
      if (rc != 0)
        return rc;

      continue;
    }

    // is this a function?
    if (std::optional<Match> match = function.match(line)) {

      Symbol s
        = make_symbol(Symbol::DEFINITION, filename, line, lineno, *match, "");
      int rc = callback(s);
      if (rc != 0)
        return rc;

      // save context we are now assumed to be within
      size_t start = match->start_offset;
      size_t extent = match->end_offset - start;
      parent = line.substr(start, extent);

      continue;
    }

    // is this a branch?
    if (std::optional<Match> match = call.match(line)) {

      Symbol s = make_symbol(Symbol::FUNCTION_CALL, filename, line, lineno,
        *match, parent);
      int rc = callback(s);
      if (rc != 0)
        return rc;

      continue;
    }
  }

  return 0;
}

}
