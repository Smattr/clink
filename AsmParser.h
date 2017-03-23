/* Generic target-agnostic assembly parser.
 *
 * "but... isn't that impossible...?"
 *
 * Well strictly yes, in the sense that different platforms do not have a common
 * assembly format. Even different assemblers do not have a common format.
 * Instead of trying to cope directly with this variation, this parser works on
 * the fuzzy notion that each line of an assembly file consists of one of the
 * following forms:
 *
 *   1. #define FOO...
 *   2. #include <...> or #include "..."
 *   3. function_name:
 *   4. branch_instruction target
 *   5. something else irrelevant
 *
 * (1) and (2) give us enough pre-processor functionality for Clink's needs. For
 * (3), the rules for what can be a function name are essentially
 * "[a-zA-Z_.][\w_.$@]*". This isn't accurate, but matches most symbols humans
 * come up with. For (4), the parser has a loose understanding of what branch
 * instructions look like on several relevant platforms. Anything else that
 * falls under (5) just gets ignored.
 *
 * The above sounds like quite a dumb approach, but you'd be surprised how well
 * it works.
 */

#pragma once

#include <cstdio>
#include <string>
#include "Parser.h"

enum AsmTokenCategory {
  ASM_NEWLINE,
  ASM_WHITESPACE,
  ASM_EOF,
  ASM_IDENTIFIER,
  ASM_STRING,
  ASM_OTHER,
};

struct AsmToken {
  AsmTokenCategory category;
  std::string text;
};

class AsmLexer {

 public:
  ~AsmLexer();
  bool load(const char *path);
  void unload();
  AsmToken next();

 private:
  FILE *m_file = nullptr;

  enum {
    IDLE,
    HASH,
    INCLUDE,
    IGNORING,
  } state = IDLE;
};

class AsmParser : public Parser {

 public:
  virtual ~AsmParser() {}

  bool load(const char *path);
  void unload();

  void process(SymbolConsumer &consumer) final;

 private:
  AsmLexer lexer;
  std::string filename;

  FILE *file;

  std::string last_line_text;
  unsigned last_line_number;

  const char *get_context(unsigned line);
};
