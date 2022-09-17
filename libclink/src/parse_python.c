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

  // check the file is readable
  if (ERROR(access(filename, R_OK) < 0))
    return errno;

  static const char *KEYWORDS[] = {
      "False",  "await", "else",     "import", "pass",   "None",    "break",
      "except", "in",    "raise",    "True",   "class",  "finally", "is",
      "return", "and",   "continue", "for",    "lambda", "try",     "as",
      "def",    "from",  "nonlocal", "while",  "assert", "del",     "global",
      "not",    "with",  "async",    "elif",   "if",     "or",      "yield",
  };
  static const size_t KEYWORDS_LENGTH = sizeof(KEYWORDS) / sizeof(KEYWORDS[0]);

  static const char *DEFN_LEADERS[] = {"class", "def"};
  static const size_t DEFN_LEADERS_LENGTH =
      sizeof(DEFN_LEADERS) / sizeof(DEFN_LEADERS[0]);

  static const clink_lang_t PYTHON = {.keywords = KEYWORDS,
                                      .keywords_length = KEYWORDS_LENGTH,
                                      .defn_leaders = DEFN_LEADERS,
                                      .defn_leaders_length =
                                          DEFN_LEADERS_LENGTH};

  return clink_parse_generic(db, filename, &PYTHON);
}
