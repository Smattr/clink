#include "AsmParser.h"
#include <cassert>
#include <ctype.h>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <unordered_set>
#include "WorkQueue.h"

using namespace std;

AsmLexer::~AsmLexer() {
  if (m_file)
    unload();
}

bool AsmLexer::load(const char *path) {
  if (m_file)
    unload();
  m_file = fopen(path, "r");
  return m_file != nullptr;
}

void AsmLexer::unload() {
  if (m_file)
    fclose(m_file);
  m_file = nullptr;
}

static bool is_identifier_char(int c) {
  return isalnum(c) || c == '_' || c == '.' || c == '$' || c == '@';
}

static bool is_whitespace_char(int c) {
  return isspace(c) && c != '\n';
}

AsmToken AsmLexer::next() {
  assert(m_file != NULL);

  AsmToken token;

  int c = fgetc(m_file);

  if (c == '\n') {
    token.category = ASM_NEWLINE;
    token.text = "\n";
    state = IDLE;
    return token;
  }

  else if (isspace(c)) {
    token.category = ASM_WHITESPACE;
    token.text += char(c);
    while (is_whitespace_char(c = fgetc(m_file)))
      token.text += char(c);
  }

  else if (c == EOF) {
    token.category = ASM_EOF;
    return token;
  }

  else if (isalpha(c) || c == '_' || c == '.') {
    token.category = ASM_IDENTIFIER;
    token.text += char(c);
    while (is_identifier_char(c = fgetc(m_file)))
      token.text += char(c);
    if (state == HASH && token.text == "include") {
      state = INCLUDE;
    } else {
      state = IGNORING;
    }
  }

  else if (state == INCLUDE && (c == '"' || c == '<')) {
    // Note that we don't care about discriminating between "..." and <...>
    token.category = ASM_STRING;
    int ender = c == '"' ? '"' : '>';
    for (;;) {
      c = fgetc(m_file);
      if (c == ender || c == EOF)
        break;
      token.text += char(c);

    }
    state = IGNORING;
    return token;
  }

  else {
    token.category = ASM_OTHER;
    token.text = char(c);
    if (state == IDLE && token.text == "#") {
      state = HASH;
    } else {
      state = IGNORING;
    }
    return token;
  }

  /* If we've read one character beyond the current token, skip back so we can
   * retrieve it next time.
   */
  if (c != EOF)
    (void)fseek(m_file, -1, SEEK_CUR);

  return token;
}

bool AsmParser::load(const char *path) {
  if (lexer.load(path)) {
    file = fopen(path, "r");
    last_line_number = 0;
    filename = path;
    return true;
  } else {
    filename = "";
    return false;
  }
}

void AsmParser::unload() {
  lexer.unload();
  filename = "";
  if (file != nullptr)
    fclose(file);
  file = nullptr;
}

/* It is expected that we move through the file linearly forwards, so calls will
 * only be made to this function with monotonically increasing arguments.
 */
const char *AsmParser::get_context(unsigned line) {
  assert(line >= last_line_number);

  if (file == nullptr)
    return "";

  char *buffer = nullptr;
  size_t dummy;
  while (line > last_line_number) {
    if (getline(&buffer, &dummy, file) < 0) {
      // EOF or error
      fclose(file);
      file = nullptr;
      return "";
    }
    last_line_text = buffer;
    last_line_number++;
  }
  free(buffer);

  return last_line_text.c_str();
}

static bool is_jump_instruction(const string &instruction) {
  static const unordered_set<string> JUMP_INSTRUCTIONS = {

    /* ARM. Note, we omit some jumps like bx that take a register, cbz that
     * are rarely used for long jumps and literal pool loads that are more
     * complex to parse.
     */
    "b", "beq", "bne", "bcs", "bhs", "bcc", "blo", "bmi", "bpl", "bvs",
    "bvc", "bhi", "bls", "bge", "blt", "bgt", "ble", "bal",
    "bl", "bleq", "blne", "blcs", "blhs", "blcc", "bllo", "blmi", "blpl",
    "blvs", "blvc", "blhi", "blls", "blge", "bllt", "blgt", "blle", "blal",
    "blx", "blxeq", "blxne", "blxcs", "blxhs", "blxcc", "blxlo", "blxmi",
    "blxpl", "blxvs", "blxvc", "blxhi", "blxls", "blxge", "blxlt", "blxgt",
    "blxle", "blxal",

    /* AVR. We omit brb{c|s} that take a bit index as the first parameter.
     */
    "brcc", "brcs", "breq", "brge", "brhc", "brhs", "brid", "brie", "brlo",
    "brlt", "brmi", "brme", "brpl", "brsh", "brtc", "brts", "brvc", "brvs",
    "jmp",  

    /* MIPS. Support here isn't great because MIPS has a set of instructions
     * that take registers to compare as the first parameters (beq and
     * friends). Parsing these requires more acrobatics than we're willing
     * to admit.
     */
    "j", "jal",

    /* PowerPC. Note that we omit bne which is a bit trickier to parse.
     */
    "b", "ba", "bl", "bla", "blt", "bdnz",

    /* RISC-V. Note we omit all the conditional branch instructions.
     */
    "jal",

    /* x86. Note that we omit some jumps like loop that are rarely used for
     * function calls.
     */
    "call", "ja", "jae", "jb", "jbe", "jc", "jcxz", "je", "jecxz", "jg",
    "jge", "jl", "jle", "jmp", "jna", "jnae", "jnb", "jnbe", "jnc", "jne",
    "jng", "jnge", "jnl", "jnle", "jno", "jnp", "jns", "jnz", "jo", "jp",
    "jpe", "jpo", "js", "jz",

  };
  return JUMP_INSTRUCTIONS.find(instruction) != JUMP_INSTRUCTIONS.end();
}

void AsmParser::process(SymbolConsumer &consumer, WorkQueue *wq) {

  enum {
    IDLE,
    INDENTED,
    JUMP,
    IGNORING,
    HASH,
    DEFINE,
    INCLUDE,
  } state = IDLE;

  unsigned line = 1;
  unsigned column = 1;

  string last_defn;

  for (;;) {

    AsmToken token = lexer.next();

    if (token.category == ASM_EOF)
      return;

    if (state == IGNORING && token.category != ASM_NEWLINE)
      continue;

    switch (token.category) {

      case ASM_NEWLINE:
        state = IDLE;
        line++;
        column = 1;
        continue;

      case ASM_WHITESPACE:
        if (state == IDLE)
          state = INDENTED;
        break;

      case ASM_EOF:
        __builtin_unreachable();

      case ASM_IDENTIFIER:
        if (state == IDLE || state == DEFINE) {
          SymbolCore s(token.text, filename, ST_DEFINITION, line, column, nullptr);
          consumer.consume(s);
          wq->push(filename);
          last_defn = token.text;
          state = IGNORING;
        } else if (state == INDENTED) {
          if (is_jump_instruction(token.text)) {
            state = JUMP;
          } else {
            state = IGNORING;
          }
        } else if (state == JUMP) {
          SymbolCore s(token.text, filename, ST_FUNCTION_CALL, line, column,
            last_defn == "" ? nullptr : last_defn.c_str());
          consumer.consume(s);
          wq->push(filename);
          state = IGNORING;
        } else if (state == HASH) {
          if (token.text == "define") {
            state = DEFINE;
          } else if (token.text == "include") {
            state = INCLUDE;
          } else {
            state = IGNORING;
          }
        } else {
          state = IGNORING;
        }
        break;

      case ASM_STRING:
        if (state == INCLUDE) {
          SymbolCore s(token.text, filename, ST_INCLUDE, line, column, nullptr);
          consumer.consume(s);
          wq->push(filename);
        }
        state = IGNORING;
        /* Account for the quotes that are not included in the contents of this
         * token.
         */
        column += 2;
        break;

      case ASM_OTHER:
        if ((state == IDLE || state == INDENTED) && token.text == "#") {
          state = HASH;
        } else {
          state = IGNORING;
        }
        break;

    }

    column += token.text.size();
  }
}
