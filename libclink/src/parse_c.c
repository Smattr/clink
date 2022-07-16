/// \file
/// \brief fuzzy C parser
///
/// The parser below uses a set of heuristics to parse C that may not be
/// compilable or even syntactically correct. This is a pretty odd thing to
/// write, so deserves an FAQ:
///
///   Why not use libclang?
///     Libclang requires a compilation database to be effective and is
///     surprisingly slow because it is trying to do a more faithful job than
///     us.
///
///   Why not use one of the open source C99 YACC grammars?
///     These mostly assume syntactic correctness of the source and, like
///     libclang, are trying to faithfully capture the full semantics of the
///     source. We do not care about, e.g., arithmetic operations.
///
///   Why not use Cscope’s code?
///     Cscope has an existing fuzzy C parser that does a pretty good job. There
///     is no good reason I did not reuse it. I was simply curious how hard it
///     would be to write one from scratch.

#include "../../common/compiler.h"
#include "debug.h"
#include "scanner.h"
#include "span.h"
#include <assert.h>
#include <clink/c.h>
#include <clink/db.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

/// does this token indicate the next token is the name of a definition?
static bool is_type(span_t token) {

  if (token.base == NULL)
    return false;

  assert(token.size > 0);

  // type leaders (type names, qualifiers, …)
  static const char *LEADERS[] = {
      "auto",       "bool",       "char",         "const",    "double",
      "enum",       "extern",     "float",        "inline",   "int",
      "long",       "register",   "restrict",     "short",    "signed",
      "static",     "struct",     "union",        "unsigned", "void",
      "volatile",   "_Atomic",    "_Bool",        "_Complex", "_Decimal128",
      "_Decimal32", "_Decimal64", "_Thread_local"};
  static const size_t LEADERS_LENGTH = sizeof(LEADERS) / sizeof(LEADERS[0]);

  for (size_t i = 0; i < LEADERS_LENGTH; ++i) {
    if (token.size != strlen(LEADERS[i]))
      continue;
    if (strncmp(token.base, LEADERS[i], token.size) != 0)
      continue;
    return true;
  }

  return false;
}

/// is this something like “struct” that precedes a type definition?
static bool is_leader(span_t token) {

  if (token.base == NULL)
    return false;

  assert(token.size > 0);

  static const char *LEADERS[] = {"enum", "struct", "union"};
  static const size_t LEADERS_LENGTH = sizeof(LEADERS) / sizeof(LEADERS[0]);

  for (size_t i = 0; i < LEADERS_LENGTH; ++i) {
    if (token.size != strlen(LEADERS[i]))
      continue;
    if (strncmp(token.base, LEADERS[i], token.size) != 0)
      continue;
    return true;
  }

  return false;
}

/// is this token a keyword?
static bool is_keyword(span_t token) {

  assert(token.base != NULL);
  assert(token.size > 0);

#define KEYWORD(x)                                                             \
  do {                                                                         \
    if (token.size != strlen(#x)) {                                            \
      break;                                                                   \
    }                                                                          \
    if (strncmp(token.base, #x, token.size) != 0) {                            \
      break;                                                                   \
    }                                                                          \
    return true;                                                               \
  } while (0);
#define C99(x) KEYWORD(x)
#define C11(x) KEYWORD(x)
#define C23(x) KEYWORD(x)
#define CXX_C(x) KEYWORD(x)
#define CXX(x)    // nothing
#define CXX_11(x) // nothing
#define CXX_20(x) // nothing
#include "c_keywords.inc"
#undef KEYWORD
#undef C99
#undef C11
#undef C23
#undef CXX_C
#undef CXX
#undef CXX_11
#undef CXX_20

  return false;
}

/// advance one character
static void eat_one(scanner_t *s) {

  assert(s->base != NULL && "corrupted scanner state");
  assert(s->offset <= s->size && "corrupted scanner state");
  assert(s->offset < s->size && "advancing an exhausted scanner");

  if (s->base[s->offset] == '\n') {
    // TODO: Windows line endings
    ++s->lineno;
    s->colno = 1;
  } else {
    ++s->colno;
  }
  ++s->offset;
}

/// advance over white space
static void eat_ws(scanner_t *s) {

  assert(s->base != NULL && "corrupted scanner state");
  assert(s->offset <= s->size && "corrupted scanner state");

  while (s->offset < s->size && isspace(s->base[s->offset]))
    eat_one(s);
}

/// advance and return true if the expected text is next
static bool eat_if(scanner_t *s, const char *expected) {

  assert(s->base != NULL && "corrupted scanner state");
  assert(s->offset <= s->size && "corrupted scanner state");
  assert(expected != NULL);
  assert(strlen(expected) > 0);
  assert(strchr(expected, '\n') == NULL && "line adjustment not supported");
  assert(strchr(expected, '\r') == NULL && "line adjustment not supported");

  if (s->size - s->offset < strlen(expected))
    return false;

  if (strncmp(&s->base[s->offset], expected, strlen(expected)) != 0)
    return false;

  s->offset += strlen(expected);
  s->colno += strlen(expected);
  return true;
}

/// check if the next characters are optional white space then the expected
static bool peek(scanner_t s, const char *expected) {

  assert(s.base != NULL && "corrupted scanner state");
  assert(s.offset <= s.size && "corrupted scanner state");
  assert(expected != NULL);
  assert(strlen(expected) > 0);
  assert(strchr(expected, '\n') == NULL && "line adjustment not supported");
  assert(strchr(expected, '\r') == NULL && "line adjustment not supported");

  eat_ws(&s);

  return eat_if(&s, expected);
}

/// state capturing our idea of the current semantic parent
typedef struct {
  size_t bracing;      ///< levels of '{' we are deep
  span_t brace_parent; ///< parent wrt '{' scope
  size_t parens;       ///< levels of '(' we are deep
  span_t paren_parent; ///< parent wrt '(' scope
} parent_t;

/// determine the current semantic parent
static const span_t *get_active_parent(const parent_t *parents) {

  assert(parents != NULL);

  // if we are inside a '{' scope, assume this is a function body
  if (parents->bracing > 0) {
    if (parents->brace_parent.base == NULL)
      return NULL;
    return &parents->brace_parent;
  }

  // if we are inside a '(' scope, assume this is a function parameter list
  if (parents->parens > 0) {
    if (parents->paren_parent.base == NULL)
      return NULL;
    return &parents->paren_parent;
  }

  return NULL;
}

int clink_parse_c(clink_db_t *db, const char *filename, size_t argc,
                  const char **argv) {

  if (UNLIKELY(db == NULL))
    return EINVAL;

  if (UNLIKELY(filename == NULL))
    return EINVAL;

  if (UNLIKELY(argc > 0 && argv == NULL))
    return EINVAL;

  int rc = 0;
  int fd = -1;
  scanner_t s = {.base = MAP_FAILED, .lineno = 1, .colno = 1};

  fd = open(filename, O_RDONLY);
  if (fd < 0) {
    rc = errno;
    goto done;
  }

  struct stat st;
  if (fstat(fd, &st) < 0) {
    rc = errno;
    goto done;
  }
  s.size = (size_t)st.st_size;

  s.base = mmap(NULL, s.size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (s.base == MAP_FAILED) {
    rc = errno;
    goto done;
  }

  // state capturing function definition we may be within
  parent_t parent = {0};

  // previous symbol seen
  span_t last = {0};

  // a symbol currently being accrued
  span_t pending = {0};

  // a symbol we will update and reuse as we parse
  clink_symbol_t symbol = {.path = (char *)filename};

  while (s.offset < s.size) {
    char c = s.base[s.offset];

    // is this character eligible to be part of a symbol?
    if (isalpha(c) || c == '_' || (pending.base != NULL && isdigit(c))) {

      // if we are starting a new symbol, note its position
      if (pending.base == NULL) {
        symbol.lineno = s.lineno;
        symbol.colno = s.colno;
        pending = (span_t){.base = &s.base[s.offset]};
      }

      ++pending.size;
    }

    // is this terminating the current symbol?
    if (pending.base != NULL &&
        ((!isalnum(c) && c != '_') || s.offset + 1 == s.size)) {

      if (!is_keyword(pending)) {

        assert(symbol.name == NULL);
        symbol.name = strndup(pending.base, pending.size);
        if (UNLIKELY(symbol.name == NULL)) {
          rc = ENOMEM;
          goto done;
        }

        {
          const span_t *symbol_parent = get_active_parent(&parent);
          if (symbol_parent != NULL) {
            symbol.parent = strndup(symbol_parent->base, symbol_parent->size);
            if (UNLIKELY(symbol.parent == NULL)) {
              free(symbol.name);
              rc = ENOMEM;
              goto done;
            }
          }
        }

        // is this an enum/struct/union definition?
        if (is_leader(last) && peek(s, "{")) {
          symbol.category = CLINK_DEFINITION;

          // is this some other kind of definition?
        } else if (last.base != NULL && !is_type(pending)) {
          symbol.category = CLINK_DEFINITION;

          // if this is a function definition, consider this our parent for any
          // upcoming symbols
          if (parent.bracing == 0 && peek(s, "(")) {
            DEBUG("entering parent \"%.*s\"", (int)pending.size, pending.base);
            parent.brace_parent = parent.paren_parent = pending;
          }

          // is this a function call?
        } else if (get_active_parent(&parent) != NULL && !is_type(pending) &&
                   eat_if(&s, "(")) {
          symbol.category = CLINK_FUNCTION_CALL;

          // otherwise consider this a reference
        } else {
          symbol.category = CLINK_REFERENCE;
        }

        DEBUG("recognised %s:%lu:%lu: %s with name \"%s\" and parent \"%s\"",
              filename, symbol.lineno, symbol.colno,
              symbol.category == CLINK_DEFINITION      ? "definition"
              : symbol.category == CLINK_FUNCTION_CALL ? "function call"
                                                       : "reference",
              symbol.name, symbol.parent == NULL ? "<none>" : symbol.parent);

        rc = clink_db_add_symbol(db, &symbol);

        free(symbol.parent);
        symbol.parent = NULL;
        free(symbol.name);
        symbol.name = NULL;

        if (rc != 0)
          goto done;
      }

      last = pending;
      pending = (span_t){0};
    }

    // if this is the start of a line comment, drain it
    // TODO: newline escapes
    if (eat_if(&s, "//")) {
      for (bool seen_newline = false; s.offset < s.size && !seen_newline;) {
        seen_newline = s.base[s.offset] == '\n';
        eat_one(&s);
      }
      continue;
    }

    // if this is the start of a multi-line comment, drain it
    if (eat_if(&s, "/*")) {
      for (; s.offset < s.size; eat_one(&s)) {
        if (eat_if(&s, "*/"))
          break;
      }
      continue;
    }

    // if this is inter-symbol punctuation, treat it as separating any modifier
    // from what it could potentially apply to, with an exception for '*' to
    // account for pointers
    if (pending.base == NULL && !isspace(c) && c != '*')
      last = (span_t){0};

    // if this is a character literal, drain it
    if (eat_if(&s, "'")) {
      while (s.offset < s.size) {
        if (eat_if(&s, "\\'"))
          continue;
        if (eat_if(&s, "\\\\"))
          continue;
        if (eat_if(&s, "'"))
          break;
        eat_one(&s);
      }
      continue;
    }

    // if this is a string literal, drain it
    if (eat_if(&s, "\"")) {
      while (s.offset < s.size) {
        if (eat_if(&s, "\\\""))
          continue;
        if (eat_if(&s, "\\\\"))
          continue;
        if (eat_if(&s, "\""))
          break;
        eat_one(&s);
      }
      continue;
    }

    // are we entering a function (overly broad match, but OK)?
    if (c == '{')
      ++parent.bracing;

    // are we leaving a function?
    if (c == '}' && parent.bracing > 0) {
      --parent.bracing;
      if (parent.bracing == 0) {
        if (parent.brace_parent.base != NULL)
          DEBUG("leaving parent \"%.*s\"", (int)parent.brace_parent.size,
                parent.brace_parent.base);
        parent.brace_parent = (span_t){0};
      }
    }

    // are we entering a function’s argument list (overly broad match, but OK)?
    if (c == '(')
      ++parent.parens;

    // are we leaving a function’s argument list?
    if (c == ')' && parent.parens > 0) {
      --parent.parens;
      if (parent.parens == 0)
        parent.paren_parent = (span_t){0};
    }

    eat_one(&s);
  }

done:
  if (s.base != MAP_FAILED)
    (void)munmap((void *)s.base, s.size);
  if (fd >= 0)
    (void)close(fd);

  return rc;
}
