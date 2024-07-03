#include "debug.h"
#include <assert.h>
#include <clink/c.h>
#include <clink/db.h>
#include <clink/generic.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>

int clink_parse_cxx(clink_db_t *db, const char *filename) {

  if (ERROR(db == NULL))
    return EINVAL;

  if (ERROR(filename == NULL))
    return EINVAL;

  if (ERROR(filename[0] != '/'))
    return EINVAL;

  // check the file is readable
  if (ERROR(access(filename, R_OK) < 0))
    return errno;

  // just enable everything
  const char *KEYWORDS[
#define C99(keyword) 1 +
#define C11(keyword) 1 +
#define C23(keyword) 1 +
#define CXX_C(keyword) 1 +
#define CXX(keyword) 1 +
#define CXX_11(keyword) 1 +
#define CXX_20(keyword) 1 +
#include "c_keywords.inc"
#undef C99
#undef C11
#undef C23
#undef CXX_C
#undef CXX
#undef CXX_11
#undef CXX_20
      1] = {0};
#define ADD(keyword)                                                           \
  for (size_t i = 0;; ++i) {                                                   \
    assert(i < sizeof(KEYWORDS) / sizeof(KEYWORDS[0]));                        \
    if (KEYWORDS[i] == NULL) {                                                 \
      KEYWORDS[i] = #keyword;                                                  \
      break;                                                                   \
    }                                                                          \
    if (strcmp(KEYWORDS[i], #keyword) == 0) {                                  \
      break;                                                                   \
    }                                                                          \
  }
#define C99(keyword) ADD(keyword)
#define C11(keyword) ADD(keyword)
#define C23(keyword) ADD(keyword)
#define CXX_C(keyword) ADD(keyword)
#define CXX(keyword) ADD(keyword)
#define CXX_11(keyword) ADD(keyword)
#define CXX_20(keyword) ADD(keyword)
#include "c_keywords.inc"
#undef C99
#undef C11
#undef C23
#undef CXX_C
#undef CXX
#undef CXX_11
#undef CXX_20
#undef ADD

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

  const clink_lang_t CXX = {
      .keywords = KEYWORDS, .defn_leaders = DEFN_LEADERS, .comments = COMMENTS};

  return clink_parse_generic(db, filename, &CXX);
}
