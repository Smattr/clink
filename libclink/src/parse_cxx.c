#include "../../common/compiler.h"
#include <clink/c.h>
#include <clink/generic.h>
#include <errno.h>
#include <unistd.h>

int clink_parse_cxx(clink_db_t *db, const char *filename) {

  if (UNLIKELY(db == NULL))
    return EINVAL;

  if (UNLIKELY(filename == NULL))
    return EINVAL;

  // check the file is readable
  if (access(filename, R_OK) < 0)
    return errno;

  static const char *KEYWORDS[] = {
  // just enable everything
#define C99(keyword) #keyword,
#define C11(keyword) #keyword,
#define C23(keyword) #keyword,
#define CXX_C(keyword) #keyword,
#define CXX(keyword) #keyword,
#define CXX_11(keyword) #keyword,
#define CXX_20(keyword) #keyword,
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
      "auto",     "bool",     "char",    "char8_t", "char16_t",
      "char32_t", "class",    "double",  "enum",    "float",
      "int",      "long",     "short",   "signed",  "struct",
      "union",    "unsigned", "wchar_t", "void",    "_Bool",
  };
  static const size_t DEFN_LEADERS_LENGTH =
      sizeof(DEFN_LEADERS) / sizeof(DEFN_LEADERS[0]);

  return clink_parse_generic(db, filename, KEYWORDS, KEYWORDS_LENGTH,
                             DEFN_LEADERS, DEFN_LEADERS_LENGTH);
}
