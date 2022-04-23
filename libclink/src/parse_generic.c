#include "../../common/compiler.h"
#include <clink/generic.h>
#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int clink_parse_generic_into(clink_db_t *db, const char *filename,
                             const char **keywords, size_t keywords_length) {

  if (UNLIKELY(db == NULL))
    return EINVAL;

  if (UNLIKELY(filename == NULL))
    return EINVAL;

  if (UNLIKELY(keywords == NULL && keywords_length > 0))
    return EINVAL;

  FILE *f = fopen(filename, "r");
  if (f == NULL)
    return errno;

  int rc = 0;

  char *pending = NULL;
  size_t pending_length = 0;
  FILE *buffer = open_memstream(&pending, &pending_length);
  if (UNLIKELY(buffer == NULL)) {
    rc = errno;
    goto done;
  }

  // do we have a non-empty pending symbol in the buffer
  bool has_pending = false;

  // current line and column position
  unsigned lineno = 1;
  unsigned colno = 1;

  // a symbol we will update and reuse as we parse
  clink_symbol_t symbol = {.category = CLINK_REFERENCE,
                           .path = (char *)filename,
                           .lineno = lineno,
                           .colno = colno};

  // was the last character we read '\r'?
  bool last_cr = false;

  while (true) {

    int c = getc(f);

    // is this character eligible to be part of a symbol?
    if (isalpha(c) || c == '_' || (has_pending && isdigit(c))) {

      // if we are starting a new symbol, note its position
      if (!has_pending) {
        symbol.lineno = lineno;
        symbol.colno = colno;
        has_pending = true;
      }

      if (UNLIKELY(putc(c, buffer) == EOF)) {
        rc = ENOMEM;
        goto done;
      }

      // otherwise consider it to terminate the current symbol
    } else if (has_pending) {
      (void)fflush(buffer);

      // is this one of the restricted keywords
      bool is_keyword = false;
      for (size_t i = 0; i < keywords_length; ++i) {
        if (strcmp(pending, keywords[i]) == 0) {
          is_keyword = true;
          break;
        }
      }

      // if it is not a keyword, insert it as a reference
      if (!is_keyword) {
        symbol.name = pending;
        if ((rc = clink_db_add_symbol(db, &symbol)))
          goto done;
      }

      has_pending = false;
      memset(pending, 0, strlen(pending));
      rewind(buffer);
    }

    // are we done?
    if (c == EOF)
      break;

    // update our position tracking
    if (c == '\r') {
      ++lineno;
      colno = 1;
      last_cr = true;
    } else {
      if (c == '\n') {
        if (last_cr) {
          // Windows line ending; do nothing
        } else {
          ++lineno;
          colno = 1;
        }
      } else {
        ++colno;
      }
      last_cr = false;
    }
  }

done:
  if (LIKELY(buffer != NULL))
    (void)fclose(buffer);
  free(pending);
  (void)fclose(f);

  return rc;
}
