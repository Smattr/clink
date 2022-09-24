#include "debug.h"
#include <clink/c.h>
#include <clink/db.h>
#include <clink/generic.h>
#include <errno.h>
#include <stdbool.h>
#include <unistd.h>

int clink_parse_cxx(clink_db_t *db, const char *filename) {

  if (ERROR(db == NULL))
    return EINVAL;

  if (ERROR(filename == NULL))
    return EINVAL;

  // check the file is readable
  if (ERROR(access(filename, R_OK) < 0))
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
      NULL};

  static const char *DEFN_LEADERS[] = {
      "auto",  "bool",   "char",   "char8_t", "char16_t", "char32_t",
      "class", "double", "enum",   "float",   "int",      "long",
      "short", "signed", "struct", "union",   "unsigned", "wchar_t",
      "void",  "_Bool",  NULL,
  };

  static clink_comment_t COMMENTS[] = {
      {.start = "//", .end = NULL, .escapes = true},
      {.start = "/*", .end = "*/", .escapes = true},
      {.start = "\"", .end = "\"", .escapes = true},
      {.start = "'", .end = "'", .escapes = true},
      {0}};

  static const clink_lang_t CXX = {
      .keywords = KEYWORDS, .defn_leaders = DEFN_LEADERS, .comments = COMMENTS};

  return clink_parse_generic(db, filename, &CXX);
}
