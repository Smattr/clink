#include "../../common/compiler.h"
#include "re.h"
#include <assert.h>
#include <regex.h>
#include <sqlite3.h>
#include <stddef.h>

void re_sqlite(sqlite3_context *context, int argc, sqlite3_value **argv) {

  assert(context != NULL);
  assert(argc == 2);
  (void)argc;
  assert(argv != NULL);

  const char *pattern = (const char *)sqlite3_value_text(argv[0]);
  assert(pattern != NULL);

  const char *text = (const char *)sqlite3_value_text(argv[1]);
  assert(text != NULL);

  regex_t regex;
  int rc = regcomp(&regex, pattern, REG_EXTENDED | REG_NOSUB);
  if (UNLIKELY(rc == REG_ESPACE)) {
    sqlite3_result_error_nomem(context);
    return;
  }
#ifdef REG_ESIZE
  if (UNLIKELY(rc == REG_ESIZE)) {
    sqlite3_result_error_toobig(context);
    return;
  }
#endif
  if (rc != 0) {
    sqlite3_result_error(context, "invalid regular expression", -1);
    return;
  }

  rc = regexec(&regex, text, 0, NULL, 0);
  regfree(&regex);

  sqlite3_result_int(context, rc == 0);
}
