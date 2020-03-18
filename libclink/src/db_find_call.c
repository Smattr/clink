#include <assert.h>
#include <clink/db.h>
#include <clink/symbol.h>
#include "error.h"
#include "sql.h"
#include <sqlite3.h>
#include <stddef.h>
#include <string.h>

int clink_db_find_call(struct clink_db *db, const char *name,
    int (*callback)(const struct clink_result *result)) {

  assert(db != NULL);
  assert(name != NULL);
  assert(callback != NULL);

  static const char QUERY[] = "select symbols.name, symbols.path, "
    "symbols.line, symbols.col, content.body from symbols left join content on "
    "symbols.path = content.path and symbols.line = content.line where "
    "symbols.parent = @parent and symbols.category = @category;";

  int rc = 0;
  sqlite3_stmt *stmt = NULL;

  if ((rc = sql_prepare(db->handle, QUERY, &stmt))) {
    rc = sqlite_error(rc);
    goto done;
  }

  if ((rc = sql_bind_text(stmt, 1, name, strlen(name)))) {
    rc = sqlite_error(rc);
    goto done;
  }

  if ((rc = sqlite3_bind_int(stmt, 2, CLINK_FUNCTION_CALL))) {
    rc = sqlite_error(rc);
    goto done;
  }

  while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {

    // construct a result for this row
    char *call = (char*)sqlite3_column_text(stmt, 0);
    const struct clink_symbol sym = {
      .category = CLINK_FUNCTION_CALL,
      .name = (char*)call,
      .name_len = strlen(call),
      .path = (char*)sqlite3_column_text(stmt, 1),
      .lineno = (unsigned long)sqlite3_column_int64(stmt, 2),
      .colno = (unsigned long)sqlite3_column_int64(stmt, 3),
      .parent = (char*)name,
    };
    const struct clink_result res = {
      .symbol = sym,
      .context = (char*)sqlite3_column_text(stmt, 4),
    };

    // pass it to the callback
    if ((rc = callback(&res)))
      goto done;
  }

  // translate any received error
  if (rc == SQLITE_DONE) {
    rc = 0;
  } else {
    rc = sqlite_error(rc);
  }

done:
  if (stmt != NULL)
    sqlite3_finalize(stmt);

  return rc;
}
