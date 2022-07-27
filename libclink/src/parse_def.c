#include "debug.h"
#include <clink/def.h>
#include <clink/generic.h>
#include <errno.h>

int clink_parse_def(clink_db_t *db, const char *filename) {

  if (ERROR(db == NULL))
    return EINVAL;

  if (ERROR(filename == NULL))
    return EINVAL;

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
      "WINDOWS",
  };
  static const size_t KEYWORDS_LENGTH = sizeof(KEYWORDS) / sizeof(KEYWORDS[0]);

  return clink_parse_generic(db, filename, KEYWORDS, KEYWORDS_LENGTH, NULL, 0);
}
