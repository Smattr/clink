#include "add_symbol.h"
#include "debug.h"
#include "isid.h"
#include "mmap.h"
#include "span.h"
#include <clink/generic.h>
#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>

int clink_parse_generic(clink_db_t *db, const char *filename,
                        const clink_lang_t *lang) {

  if (ERROR(db == NULL))
    return EINVAL;

  if (ERROR(filename == NULL))
    return EINVAL;

  if (ERROR(lang == NULL))
    return EINVAL;

  int rc = 0;
  mmap_t mapped = {0};

  rc = mmap_open(&mapped, filename);
  if (ERROR(rc != 0))
    goto done;

  // if this is a zero-sized file, nothing to be done
  if (mapped.size == 0)
    goto done;

  const char *base = mapped.base;
  size_t size = mapped.size;

  // a symbol we are accruing
  span_t pending = {0};

  // current line and column position
  unsigned lineno = 1;
  unsigned colno = 1;

  // was the last token we read a definition leader?
  bool last_defn_leader = false;

  for (size_t j = 0; j < size;) {

    int c = base[j];

    // is this the start of a new identifier?
    if (pending.base == NULL && isid0(c)) {

      // note its position
      pending = (span_t){.base = &base[j], .lineno = lineno, .colno = colno};
    }

    // is this part of the current identifier?
    if (pending.base != NULL && isid(c))
      ++pending.size;

    // is this the end of the current identifier?
    if (pending.base != NULL && (j + 1 == size || !isid(base[j + 1]))) {

      // is this one of the restricted keywords?
      bool is_keyword = false;
      if (lang->keywords != NULL) {
        for (size_t i = 0; lang->keywords[i] != NULL; ++i) {
          if (span_eq(pending, lang->keywords[i])) {
            is_keyword = true;
            break;
          }
        }
      }

      // is this a definition leader?
      bool is_defn_leader = false;
      if (lang->defn_leaders != NULL) {
        for (size_t i = 0; lang->defn_leaders[i] != NULL; ++i) {
          if (span_eq(pending, lang->defn_leaders[i])) {
            is_defn_leader = true;
            break;
          }
        }
      }

      // if it is not a keyword, insert it
      if (!is_keyword) {

        // assume a definition leader itself cannot be a definition
        clink_category_t category = CLINK_REFERENCE;
        if (last_defn_leader && !is_defn_leader) {
          category = CLINK_DEFINITION;
        }

        const span_t no_parent = {0};
        if ((rc = add_symbol(db, category, pending, filename, no_parent)))
          goto done;
      }

      // Does this indicate the next symbol is a definition? Note that both
      // keywords and non-keywords can be definition leaders.
      last_defn_leader = is_defn_leader;

      pending = (span_t){0};

      ++colno;
      ++j;
      continue;
    }

    // if this is something other than whitespace, it separates a definition
    // leader from anything it could apply to
    if (pending.base == NULL && !isspace(c))
      last_defn_leader = false;

    // update our position tracking
    if (c == '\r') {
      if (j + 1 < size && base[j + 1] == '\n') // Windows line ending
        ++j;
      ++lineno;
      colno = 1;
    } else if (c == '\n') {
      ++lineno;
      colno = 1;
    } else {
      ++colno;
    }
    ++j;
  }

done:
  mmap_close(mapped);

  return rc;
}
