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
#include "add_symbol.h"
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

/// language being processed
typedef enum {
  CPP, // C pre-processor
  C23, // GNU C23
} lang_t;

/// is this a character that prevents the prior symbol applying (e.g. as a
/// qualifier) to the next symbol?
static bool is_blocker(lang_t lang, char c) {

  if (isspace(c))
    return false;

  // allow for pointer definitions
  if (lang == C23 && c == '*')
    return false;

  return true;
}

/// is this a pre-processor directive?
static bool is_directive(lang_t lang, span_t token) {

  assert(token.base != NULL);

  if (lang != CPP)
    return false;

  if (span_eq(token, "define"))
    return true;
  if (span_eq(token, "elif"))
    return true;
  if (span_eq(token, "else"))
    return true;
  if (span_eq(token, "endif"))
    return true;
  if (span_eq(token, "error"))
    return true;
  if (span_eq(token, "if"))
    return true;
  if (span_eq(token, "ifdef"))
    return true;
  if (span_eq(token, "ifndef"))
    return true;
  if (span_eq(token, "include"))
    return true;
  if (span_eq(token, "line"))
    return true;
  if (span_eq(token, "pragma"))
    return true;
  if (span_eq(token, "undef"))
    return true;

  return false;
}

/// is this a type-like token we can safely ignore?
static bool is_qualifier(lang_t lang, span_t token) {

  assert(token.base != NULL);

  if (lang == CPP)
    return false;

  if (span_eq(token, "const"))
    return true;
  if (span_eq(token, "extern"))
    return true;
  if (span_eq(token, "inline"))
    return true;
  if (span_eq(token, "__inline__")) // GCC extension
    return true;
  if (span_eq(token, "register"))
    return true;
  if (span_eq(token, "restrict"))
    return true;
  if (span_eq(token, "__restrict__")) // GCC extension
    return true;
  if (span_eq(token, "static"))
    return true;
  if (span_eq(token, "volatile"))
    return true;
  if (span_eq(token, "_Atomic"))
    return true;
  if (span_eq(token, "_Noreturn"))
    return true;
  if (span_eq(token, "_Thread_local"))
    return true;
  if (span_eq(token, "__thread")) // GCC extension
    return true;

  return false;
}

/// is this something like “struct” that precedes a type definition?
static bool is_leader(lang_t lang, span_t token) {

  if (token.base == NULL)
    return false;

  assert(token.size > 0);

  if (lang == C23) {
    if (span_eq(token, "enum"))
      return true;
    if (span_eq(token, "struct"))
      return true;
    if (span_eq(token, "union"))
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
    if (span_eq(token, #x)) {                                                  \
      return true;                                                             \
    }                                                                          \
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

  if (s->base[s->offset] == '\r') {
    if (s->offset + 1 < s->size && s->base[s->offset + 1] == '\n') {
      // Windows line ending
      ++s->offset;
    }
    ++s->lineno;
    s->colno = 1;
  } else if (s->base[s->offset] == '\n') {
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

static bool isspace_not_eol(char c) {
  return isspace(c) && c != '\n' && c != '\r';
}

/// advance over white space until the end of the line
static void eat_ws_to_eol(scanner_t *s) {

  assert(s->base != NULL && "corrupted scanner state");
  assert(s->offset <= s->size && "corrupted scanner state");

  while (s->offset < s->size && isspace_not_eol(s->base[s->offset]))
    eat_one(s);
}

/// advance and return true if the expected text is next
static bool eat_if(scanner_t *s, const char *expected) {

  assert(s->base != NULL && "corrupted scanner state");
  assert(s->offset <= s->size && "corrupted scanner state");
  assert(expected != NULL);
  assert(strlen(expected) > 0);

  if (s->size - s->offset < strlen(expected))
    return false;

  if (strncmp(&s->base[s->offset], expected, strlen(expected)) != 0)
    return false;

  s->offset += strlen(expected);

  for (size_t i = 0; i < strlen(expected); ++i) {
    if (strncmp(&expected[i], "\r\n", strlen("\r\n")) == 0) {
      ++i;
      ++s->lineno;
      s->colno = 1;
    } else if (expected[i] == '\n') {
      ++s->lineno;
      s->colno = 1;
    } else {
      ++s->colno;
    }
  }

  return true;
}

/// check if the next characters are optional white space then the expected
static bool peek(scanner_t s, const char *expected) {

  assert(s.base != NULL && "corrupted scanner state");
  assert(s.offset <= s.size && "corrupted scanner state");
  assert(s.offset < s.size && "peeking an exhausted scanner");
  assert(expected != NULL);
  assert(strlen(expected) > 0);

  eat_one(&s);
  eat_ws(&s);

  return eat_if(&s, expected);
}

/// check if the current characters are optional white space then the expected
static bool at(scanner_t s, const char *expected) {

  assert(s.base != NULL && "corrupted scanner state");
  assert(s.offset <= s.size && "corrupted scanner state");
  assert(s.offset < s.size && "examining an exhausted scanner");
  assert(expected != NULL);
  assert(strlen(expected) > 0);

  eat_ws_to_eol(&s);
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

/// is this an identifier starter?
static bool isid0(int c) { return isalpha(c) || c == '_'; }

/// is this an identifier continuer?
static bool isid(int c) { return isid0(c) || isdigit(c); }

static int parse(lang_t lang, clink_db_t *db, const char *filename,
                 scanner_t *s) {

  assert(db != NULL);
  assert(filename != NULL);
  assert(s != NULL);

  // pre-processor parsing should only be invoked if we saw a directive
  if (lang == CPP)
    assert(at(*s, "#"));

  // state capturing function definition we may be within
  parent_t parent = {0};

  // previous symbol seen
  span_t last = {0};

  // a symbol currently being accrued
  span_t pending = {0};
  unsigned long pending_lineno;
  unsigned long pending_colno;

  // have we seen a pre-processor directive yet? (only relevant for CPP)
  bool seen_directive = false;

  // do we think we are on the LHS of an expression?
  bool is_lhs = true;

  while (s->offset < s->size) {

    // if we are at the beginning of a line and see a pre-processor directive,
    // invoke the pre-processor parser
    if (lang != CPP && s->colno == 1 && at(*s, "#")) {
      int rc = parse(CPP, db, filename, s);
      if (rc != 0)
        return rc;
      continue;
    }

    char c = s->base[s->offset];

    // is this the start of a new identifier?
    if (pending.base == NULL && isid0(c)) {

      // note its position
      assert(pending.base == NULL);
      pending_lineno = s->lineno;
      pending_colno = s->colno;
      pending = (span_t){.base = &s->base[s->offset]};
    }

    // is this part of the current identifier?
    if (pending.base != NULL && isid(c))
      ++pending.size;

    // is this the end of the current identifier?
    if (pending.base != NULL &&
        (s->offset + 1 == s->size || !isid(s->base[s->offset + 1]))) {

      // if this is a qualifier, ignore it, including leaving `last` intact
      if (is_qualifier(lang, pending)) {
        pending = (span_t){0};
        eat_one(s);
        continue;
      }

      if (!is_keyword(pending)) {

        // reference is the default category if we cannot more precisely
        // identify what type of symbol this is
        clink_category_t category = CLINK_REFERENCE;

        span_t symbol_parent = {0};
        {
          const span_t *p = get_active_parent(&parent);
          if (p != NULL)
            symbol_parent = *p;
        }

        // is this an enum/struct/union definition?
        if (is_leader(lang, last) && peek(*s, "{")) {
          category = CLINK_DEFINITION;

          // is this some other kind of definition?
        } else if (last.base != NULL && is_lhs) {
          category = CLINK_DEFINITION;

          // if this is a function definition, consider this our parent for any
          // upcoming symbols
          if (lang == C23 && parent.bracing == 0 && peek(*s, "(")) {
            DEBUG("entering parent \"%.*s\"", (int)pending.size, pending.base);
            parent.brace_parent = parent.paren_parent = pending;
          }

          // for the pre-processor, the first token we see after #define is the
          // parent
          if (lang == CPP && parent.brace_parent.base == NULL &&
              span_eq(last, "define")) {
            DEBUG("entering parent \"%.*s\"", (int)pending.size, pending.base);
            parent.brace_parent = parent.paren_parent = pending;
            // pretend we are already in a braced scope to activate this parent
            assert(parent.bracing == 0);
            ++parent.bracing;
          }

          // is this a function call?
        } else if (get_active_parent(&parent) != NULL && peek(*s, "(")) {
          category = CLINK_FUNCTION_CALL;
        }

        if (seen_directive || !is_directive(lang, pending)) {
          DEBUG("%s parser recognised %s:%lu:%lu: %s with name \"%.*s\" and "
                "parent \"%.*s\"",
                lang == CPP ? "CPP" : "C23", filename, pending_lineno,
                pending_colno,
                category == CLINK_DEFINITION      ? "definition"
                : category == CLINK_FUNCTION_CALL ? "function call"
                                                  : "reference",
                (int)pending.size, pending.base,
                (int)(symbol_parent.base == NULL ? strlen("<none>")
                                                 : symbol_parent.size),
                symbol_parent.base == NULL ? "<none>" : symbol_parent.base);

          int rc = add_symbol(db, category, pending, filename, pending_lineno,
                              pending_colno, symbol_parent);
          if (rc != 0)
            return rc;
        }
      }

      if (lang == CPP) {
        // only allow application of the first directive we see, and only the
        // directives we interpret semantically
        bool known = span_eq(pending, "define") || span_eq(pending, "include");
        if (!seen_directive && known) {
          last = pending;
        } else {
          last = (span_t){0};
        }
        seen_directive = true;
      } else {
        last = pending;
      }

      pending = (span_t){0};

      eat_one(s);
      continue;
    }

    // if this is the start of a line comment, drain it
    // TODO: newline escapes
    if (eat_if(s, "//")) {
      for (bool seen_newline = false; s->offset < s->size && !seen_newline;) {
        seen_newline = s->base[s->offset] == '\n' || s->base[s->offset] == '\r';
        eat_one(s);
      }
      if (lang == CPP) {
        DEBUG("leaving CPP parser following // comment on line %zu",
              s->lineno - 1);
        return 0;
      }
      continue;
    }

    // if this is the start of a multi-line comment, drain it
    if (eat_if(s, "/*")) {
      for (; s->offset < s->size; eat_one(s)) {
        if (eat_if(s, "*/"))
          break;
      }
      continue;
    }

    // is this the argument to a pre-processor #include?
    if (lang == CPP && pending.base == NULL && last.base != NULL &&
        span_eq(last, "include")) {
      if (eat_if(s, "\"") || eat_if(s, "<")) {
        unsigned long line = s->lineno;
        unsigned long col = s->colno;
        assert(s->offset > 0);
        const char *ender = s->base[s->offset - 1] == '"' ? "\"" : ">";
        span_t path = {.base = &s->base[s->offset]};
        while (!eat_if(s, ender)) {
          if (s->offset == s->size)
            break;
          if (s->base[s->offset] == '\n' || s->base[s->offset] == '\r')
            break;
          eat_one(s);
          ++path.size;
        }
        const span_t no_parent = {0};
        DEBUG("CPP parser recognised %s:%lu:%lu: include with name \"%.*s\"",
              filename, line, col, (int)path.size, path.base);
        int rc =
            add_symbol(db, CLINK_INCLUDE, path, filename, line, col, no_parent);
        if (rc != 0)
          return rc;
        last = (span_t){0};
        continue;
      }
    }

    // if this is inter-symbol punctuation, treat it as separating any modifier
    // from what it could potentially apply to
    if (pending.base == NULL && is_blocker(lang, c))
      last = (span_t){0};

    // if this is a character literal, drain it
    if (eat_if(s, "'")) {
      while (s->offset < s->size) {
        if (eat_if(s, "\\'"))
          continue;
        if (eat_if(s, "\\\\"))
          continue;
        if (eat_if(s, "'"))
          break;
        eat_one(s);
      }
      continue;
    }

    // if this is a string literal, drain it
    if (eat_if(s, "\"")) {
      while (s->offset < s->size) {
        if (eat_if(s, "\\\""))
          continue;
        if (eat_if(s, "\\\\"))
          continue;
        if (eat_if(s, "\""))
          break;
        eat_one(s);
      }
      continue;
    }

    if (lang != CPP) {
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

      // are we entering a function’s argument list (overly broad match, but
      // OK)?
      if (c == '(')
        ++parent.parens;

      // are we leaving a function’s argument list?
      if (c == ')' && parent.parens > 0) {
        --parent.parens;
        if (parent.parens == 0)
          parent.paren_parent = (span_t){0};
      }
    }

    // update our guess of which side of the containing expression we are on
    if (c == '=') {
      is_lhs = false;
    } else if (c == ';') {
      is_lhs = true;
    }

    // does this end the current C pre-processor line?
    if (lang == CPP && (eat_if(s, "\r\n") || eat_if(s, "\n"))) {
      DEBUG("leaving CPP parser following newline ending line %zu\n",
            s->lineno - 1);
      return 0;
    }

    eat_one(s);
  }

  return 0;
}

int clink_parse_c(clink_db_t *db, const char *filename) {

  if (UNLIKELY(db == NULL))
    return EINVAL;

  if (UNLIKELY(filename == NULL))
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

  rc = parse(C23, db, filename, &s);
  if (rc != 0)
    goto done;

done:
  if (s.base != MAP_FAILED)
    (void)munmap((void *)s.base, s.size);
  if (fd >= 0)
    (void)close(fd);

  return rc;
}
