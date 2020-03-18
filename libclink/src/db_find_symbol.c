#include <assert.h>
#include <clink/db.h>
#include <clink/symbol.h>
#include "error.h"
#include "sql.h"
#include <sqlite3.h>
#include <stddef.h>
#include <string.h>

int clink_db_find_symbol(struct clink_db *db, const char *name,
  int (*callback)(const struct clink_result *result)) {

  assert(db != NULL);
  assert(name != NULL);
  assert(callback != NULL);

  static const char QUERY[] = "select symbols.path, symbols.category, "
    "symbols.line, symbols.col, symbols.parent, content.body from symbols left "
    "join content on symbols.path = content.path and symbols.line = "
    "content.line where symbols.name = @name;";

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

  while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {

    // construct a result for this row
    const struct clink_symbol sym = {
      .category = (enum clink_category)sqlite3_column_int(stmt, 1),
      .name = (char*)name,
      .name_len = strlen(name),
      .path = (char*)sqlite3_column_text(stmt, 0),
      .lineno = (unsigned long)sqlite3_column_int64(stmt, 2),
      .colno = (unsigned long)sqlite3_column_int64(stmt, 3),
      .parent = (char*)sqlite3_column_text(stmt, 4),
    };
    const struct clink_result res = {
      .symbol = sym,
      .context = (char*)sqlite3_column_text(stmt, 5),
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
