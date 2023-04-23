/// \file
/// \brief fuzzy Python parser

#include "debug.h"
#include <clink/db.h>
#include <clink/generic.h>
#include <clink/python.h>
#include <errno.h>
#include <stddef.h>
#include <unistd.h>

int clink_parse_python(clink_db_t *db, const char *filename) {

  if (ERROR(db == NULL))
    return EINVAL;

  if (ERROR(filename == NULL))
    return EINVAL;

  if (ERROR(filename[0] != '/'))
    return EINVAL;

  // check the file is readable
  if (ERROR(access(filename, R_OK) < 0))
    return errno;

  static const char *KEYWORDS[] = {
      "False",   "await",  "else",   "import", "pass",     "None",
      "break",   "except", "in",     "raise",  "True",     "class",
      "finally", "is",     "return", "and",    "continue", "for",
      "lambda",  "try",    "as",     "def",    "from",     "nonlocal",
      "while",   "assert", "del",    "global", "not",      "with",
      "async",   "elif",   "if",     "or",     "yield",    NULL,
  };

  static const char *DEFN_LEADERS[] = {"class", "def", NULL};

  static clink_comment_t COMMENTS[] = {{.start = "#", .end = NULL}, {0}};

  static const clink_lang_t PYTHON = {
      .keywords = KEYWORDS, .defn_leaders = DEFN_LEADERS, .comments = COMMENTS};

  return clink_parse_generic(db, filename, &PYTHON);
}
