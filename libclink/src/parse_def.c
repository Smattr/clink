#include "debug.h"
#include <clink/def.h>
#include <clink/generic.h>
#include <errno.h>
#include <unistd.h>

int clink_parse_def(clink_db_t *db, const char *filename) {

  if (ERROR(db == NULL))
    return EINVAL;

  if (ERROR(filename == NULL))
    return EINVAL;

  // check the file is readable
  if (ERROR(access(filename, R_OK) < 0))
    return errno;

  // The keywords in the Module-Definition language, according to Microsoft.
  // Technically one of these can be used as a symbol if you enclose it in
  // quotes, but we leave this for a future improvement.
  static const char *KEYWORDS[] = {
      "APPLOADER",      "BASE",
      "CODE",           "CONFORMING",
      "DATA",           "DESCRIPTION",
      "DEV386",         "DISCARDABLE",
      "DYNAMIC",        "EXECUTE-ONLY",
      "EXECUTEONLY",    "EXECUTEREAD",
      "EXETYPE",        "EXPORTS",
      "FIXED",          "FUNCTIONS",
      "HEAPSIZE",       "IMPORTS",
      "IMPURE",         "INCLUDE",
      "INITINSTANCE",   "IOPL",
      "LIBRARY",        "LOADONCALL",
      "LONGNAMES",      "MOVABLE",
      "MOVEABLE",       "MULTIPLE",
      "NAME",           "NEWFILES",
      "NODATA",         "NOIOPL",
      "NONAME",         "NONCONFORMING",
      "NONDISCARDABLE", "NONE",
      "NONSHARED",      "NOTWINDOWCOMPAT",
      "OBJECTS",        "OLD",
      "PRELOAD",        "PRIVATE",
      "PROTMODE",       "PURE",
      "READONLY",       "READWRITE",
      "REALMODE",       "RESIDENT",
      "RESIDENTNAME",   "SECTIONS",
      "SEGMENTS",       "SHARED",
      "SINGLE",         "STACKSIZE",
      "STUB",           "VERSION",
      "WINDOWAPI",      "WINDOWCOMPAT",
      "WINDOWS",        NULL,
  };

  static clink_comment_t COMMENTS[] = {{.start = ";", .end = NULL}, {0}};

  static const clink_lang_t DEF = {.keywords = KEYWORDS, .comments = COMMENTS};

  return clink_parse_generic(db, filename, &DEF);
}
