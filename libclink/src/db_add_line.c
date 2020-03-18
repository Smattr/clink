#include <assert.h>
#include <clink/db.h>
#include <errno.h>
#include "error.h"
#include "sql.h"
#include <sqlite3.h>
#include <stddef.h>
#include <string.h>

int clink_db_add_line(struct clink_db *db, const char *path,
    const char *line, unsigned long lineno) {

  assert(db != NULL);
  assert(path != NULL);
  assert(line != NULL);

  if (lineno == 0)
    return EINVAL;

  static const char INSERT[] = "insert into content (path, line, body) "
    "values (@path, @line, @body);";

  int rc = 0;
  sqlite3_stmt *stmt = NULL;

  if ((rc = sql_prepare(db->handle, INSERT, &stmt))) {
    rc = sqlite_error(rc);
    goto done;
  }

  if ((rc = sql_bind_text(stmt, "@path", 1, path, strlen(path)))) {
    rc = sqlite_error(rc);
    goto done;
  }

  if ((rc = sql_bind_int(stmt, "@line", 2, lineno))) {
    rc = sqlite_error(rc);
    goto done;
  }

  if ((rc = sql_bind_text(stmt, "@body", 3, line, strlen(line)))) {
    rc = sqlite_error(rc);
    goto done;
  }

  int result = sqlite3_step(stmt);
  if (!sql_ok(result)) {
    rc = sqlite_error(result);
    goto done;
  }

done:
  if (stmt != NULL)
    sqlite3_finalize(stmt);

  return rc;
}
