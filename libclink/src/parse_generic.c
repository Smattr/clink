#include "add_symbol.h"
#include "debug.h"
#include "isid.h"
#include "mmap.h"
#include "scanner.h"
#include "span.h"
#include <clink/generic.h>
#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>

static int parse(clink_db_t *db, const char *filename, const clink_lang_t *lang,
                 scanner_t s) {

  int rc = 0;

  // was the last token we read a definition leader?
  bool last_defn_leader = false;

  while (s.offset < s.size) {

    // is this an identifier?
    if (isid0(s.base[s.offset])) {
      span_t id = {.base = &s.base[s.offset],
                   .size = 1,
                   .lineno = s.lineno,
                   .colno = s.colno};
      for (eat_one(&s); s.offset < s.size && isid(s.base[s.offset]);
           eat_one(&s))
        ++id.size;

      // is this one of the restricted keywords?
      bool is_keyword = false;
      if (lang->keywords != NULL) {
        for (size_t i = 0; lang->keywords[i] != NULL; ++i) {
          if (span_eq(id, lang->keywords[i])) {
            is_keyword = true;
            break;
          }
        }
      }

      // is this a definition leader?
      bool is_defn_leader = false;
      if (lang->defn_leaders != NULL) {
        for (size_t i = 0; lang->defn_leaders[i] != NULL; ++i) {
          if (span_eq(id, lang->defn_leaders[i])) {
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
        if ((rc = add_symbol(db, category, id, filename, no_parent)))
          goto done;
      }

      // Does this indicate the next symbol is a definition? Note that both
      // keywords and non-keywords can be definition leaders.
      last_defn_leader = is_defn_leader;

      continue;
    }

    // is this a comment?
    if (lang->comments != NULL) {
      bool is_comment = false;
      for (size_t i = 0; lang->comments[i].start != NULL; ++i) {
        if (!eat_if(&s, lang->comments[i].start))
          continue;
        is_comment = true;
        const char *end = lang->comments[i].end;
        bool escapes = lang->comments[i].escapes;
        while (s.offset < s.size) {
          if (escapes && eat_if(&s, "\\")) {
            if (s.offset < s.size)
              eat_one(&s);
            continue;
          }
          if (end == NULL) {
            if (eat_eol(&s))
              break;
          } else {
            if (eat_if(&s, end))
              break;
          }
          eat_one(&s);
        }
        break;
      }
      if (is_comment)
        continue;
    }

    // if this is something other than whitespace, it separates a definition
    // leader from anything it could apply to
    if (!isspace(s.base[s.offset]))
      last_defn_leader = false;

    eat_one(&s);
  }

done:
  return rc;
}

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

  scanner_t s = scanner(mapped.base, mapped.size);
  rc = parse(db, filename, lang, s);
  if (ERROR(rc != 0))
    goto done;

done:
  mmap_close(mapped);

  return rc;
}
