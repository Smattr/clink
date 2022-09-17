/// \file
/// \brief fuzzy C parser

#include "debug.h"
#include <clink/c.h>
#include <clink/db.h>
#include <clink/generic.h>
#include <errno.h>
#include <stddef.h>
#include <unistd.h>

int clink_parse_c(clink_db_t *db, const char *filename) {

  if (ERROR(db == NULL))
    return EINVAL;

  if (ERROR(filename == NULL))
    return EINVAL;

  // check the file is readable
  if (ERROR(access(filename, R_OK) < 0))
    return errno;

  static const char *KEYWORDS[] = {
  // enable all C keywords
#define C99(keyword) #keyword,
#define C11(keyword) #keyword,
#define C23(keyword) #keyword,
#define CXX_C(keyword) #keyword,
#define CXX(keyword)    /* nothing */
#define CXX_11(keyword) /* nothing */
#define CXX_20(keyword) /* nothing */
#include "c_keywords.inc"
#undef C99
#undef C11
#undef C23
#undef CXX_C
#undef CXX
#undef CXX_11
#undef CXX_20
  };
  static const size_t KEYWORDS_LENGTH = sizeof(KEYWORDS) / sizeof(KEYWORDS[0]);

  static const char *DEFN_LEADERS[] = {
      "auto", "bool",  "char",   "double", "enum",  "float",    "int",
      "long", "short", "signed", "struct", "union", "unsigned", "void",
  };
  static const size_t DEFN_LEADERS_LENGTH =
      sizeof(DEFN_LEADERS) / sizeof(DEFN_LEADERS[0]);

  return clink_parse_generic(db, filename, KEYWORDS, KEYWORDS_LENGTH,
                             DEFN_LEADERS, DEFN_LEADERS_LENGTH);
}
