#include "../../common/compiler.h"
#include "re.h"
#include <assert.h>
#include <regex.h>
#include <sqlite3.h>
#include <stdatomic.h>
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

  // retrieve the pre-compiled regex state that was setup by `db_open`
  const re_t *_Atomic *re = sqlite3_user_data(context);
  assert(re != NULL);

  // find our matching regex which should have been pre-compiled
  regex_t regex = re_find(re, pattern);

  int rc = regexec(&regex, text, 0, NULL, 0);

  sqlite3_result_int(context, rc == 0);
}
