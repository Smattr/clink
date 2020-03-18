#include <assert.h>
#include <clink/db.h>
#include <errno.h>
#include "error.h"
#include "sql.h"
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int clink_db_find_file(struct clink_db *db, const char *name,
    int (*callback)(const char *path, void *state),
    void *callback_state) {

  assert(db != NULL);
  assert(name != NULL);
  assert(callback != NULL);

  int rc = 0;
  sqlite3_stmt *stmt = NULL;

  char *path = NULL;
  if (asprintf(&path, "%%/%s", name) < 0) {
    rc = errno;
    goto done;
  }

  static const char QUERY[] = "select distinct path from symbols where path = "
    "@path1 or path like @path2;";

  if ((rc = sql_prepare(db->handle, QUERY, &stmt))) {
    rc = sqlite_error(rc);
    goto done;
  }

  if ((rc = sql_bind_text(stmt, "@path1", 1, name, strlen(name)))) {
    rc = sqlite_error(rc);
    goto done;
  }

  if ((rc = sql_bind_text(stmt, "@path2", 2, path, strlen(path)))) {
    rc = sqlite_error(rc);
    goto done;
  }

  while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {

    // extract this result
    const char *res = (char*)sqlite3_column_text(stmt, 0);

    // pass it to the callback
    if ((rc = callback(res, callback_state)))
      goto done;
  }

  // translate any received error
  if (rc == SQLITE_DONE) {
    rc = 0;
  } else {
    rc = sqlite_error(rc);
  }

done:
  free(path);
  if (stmt != NULL)
    sqlite3_finalize(stmt);

  return rc;
}
