#include "add_symbol.h"
#include "debug.h"
#include "isid.h"
#include "mmap.h"
#include "scanner.h"
#include <assert.h>
#include <clink/c.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>

static int parse(clink_db_t *db, const char *filename, scanner_t s) {

  int rc = 0;

  // are we currently within a preprocessor directive?
  bool in_preproc = false;

  while (s.offset < s.size) {

    // can we enter preprocessor directive state?
    if (!in_preproc) {
      do {
        if (s.colno != 1)
          break;
        eat_ws(&s);
        if (!eat_if(&s, "#"))
          break;
        eat_ws_to_eol(&s);
        if (!(eat_id(&s, "ifdef") || eat_id(&s, "if") || eat_id(&s, "elif") ||
              eat_id(&s, "ifndef") || eat_id(&s, "undef")))
          break;
        DEBUG("entering preprocessor state on line %lu", s.lineno);
        in_preproc = true;
      } while (0);
      if (in_preproc)
        continue;
    }

    // do we have a symbol?
    if (in_preproc && isid0(s.base[s.offset])) {
      span_t sym = {.base = &s.base[s.offset],
                    .size = 1,
                    .lineno = s.lineno,
                    .colno = s.colno};

      for (eat_one(&s); s.offset < s.size && isid(s.base[s.offset]);
           eat_one(&s))
        ++sym.size;

      span_t no_parent = {0};
      rc = add_symbol(db, CLINK_REFERENCE, sym, filename, no_parent);
      if (ERROR(rc != 0))
        goto done;

      continue;
    }

    // If this is a string literal, drain it with C/C++ rules. This is slightly
    // different to CPP rules which do not comprehend escapes.
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

    // if this is a line comment, drain it and exit this preprocessor directive
    if (eat_if(&s, "//")) {
      for (bool seen_newline = false; s.offset < s.size && !seen_newline;) {
        seen_newline = s.base[s.offset] == '\n' || s.base[s.offset] == '\r';
        eat_one(&s);
      }
      in_preproc = false;
      continue;
    }

    // if this is a multi-line comment, drain it and stay in the current state
    if (eat_if(&s, "/*")) {
      for (; s.offset < s.size; eat_one(&s)) {
        if (eat_if(&s, "*/"))
          break;
      }
      continue;
    }

    // can we exit preprocessor directive state?
    if (in_preproc && eat_eol(&s)) {
      DEBUG("leaving preprocessor state on line %lu", s.lineno - 1);
      in_preproc = false;
      continue;
    }

    // otherwise, a character we do not care about
    eat_one(&s);
  }

done:
  return rc;
}

int clink_parse_cpp(clink_db_t *db, const char *filename) {

  if (ERROR(db == NULL))
    return EINVAL;

  if (ERROR(filename == NULL))
    return EINVAL;

  int rc = 0;
  mmap_t mapped = {0};

  // open the input source
  rc = mmap_open(&mapped, filename);
  if (ERROR(rc != 0))
    goto done;

  // if this is a zero-sized file, nothing to be done
  if (mapped.size == 0)
    goto done;

  // run the parser
  scanner_t s = {
      .base = mapped.base, .size = mapped.size, .lineno = 1, .colno = 1};
  rc = parse(db, filename, s);
  if (ERROR(rc != 0))
    goto done;

done:
  mmap_close(mapped);

  return rc;
}
