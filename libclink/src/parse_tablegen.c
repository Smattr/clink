/// \file
/// \brief fuzzy TableGen parser
///
/// There is no support for recognising bang operators or variables beginning
/// with $ or a digit.

#include "debug.h"
#include <clink/db.h>
#include <clink/generic.h>
#include <clink/tablegen.h>
#include <errno.h>
#include <stddef.h>
#include <unistd.h>

int clink_parse_tablegen(clink_db_t *db, const char *filename) {

  if (ERROR(db == NULL))
    return EINVAL;

  if (ERROR(filename == NULL))
    return EINVAL;

  // check the file is readable
  if (ERROR(access(filename, R_OK) < 0))
    return errno;

  static const char *KEYWORDS[] = {
      "assert", "bit",   "bits",    "class", "code",   "dag",    "def",
      "else",   "false", "foreach", "defm",  "defset", "defvar", "field",
      "if",     "in",    "include", "int",   "let",    "list",   "multiclass",
      "string", "then",  "true",    NULL};

  static const char *DEFN_LEADERS[] = {"bit",    "class",  "def", "defm",
                                       "defset", "defvar", "let", "multiclass",
                                       "string", NULL};

  // TableGen supports two string delimiters, but we omit both of them. Though
  // strings in TableGen are in theory uninterpreted, most TableGen strings in
  // the wild contain semantically interesting information.
  static clink_comment_t COMMENTS[] = {
      {.start = "//", .end = NULL, .escapes = true},
      {.start = "/*", .end = "*/", .escapes = true},
      {0}};

  static const clink_lang_t TABLEGEN = {
      .keywords = KEYWORDS, .defn_leaders = DEFN_LEADERS, .comments = COMMENTS};

  return clink_parse_generic(db, filename, &TABLEGEN);
}
