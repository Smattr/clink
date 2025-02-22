#include "db.h"
#include "debug.h"
#include "get_id.h"
#include "sql.h"
#include <clink/db.h>
#include <errno.h>
#include <sqlite3.h>
#include <stddef.h>
#include <string.h>

int clink_db_get_content(clink_db_t *db, const char *path, unsigned long lineno,
                         char **content) {

  if (ERROR(db == NULL))
    return EINVAL;

  if (ERROR(path == NULL))
    return EINVAL;

  if (ERROR(path[0] != '/'))
    return EINVAL;

  if (ERROR(content == NULL))
    return EINVAL;

  static const char QUERY[] =
      "select body from content where path = @path and line = @line;";

  int rc = 0;
  sqlite3_stmt *stmt = NULL;

  // find the identifier for the given path
  clink_record_id_t id = -1;
  if (ERROR((rc = get_id(db, path, &id))))
    goto done;

  // create a query to lookup the given file content
  if (ERROR((rc = sql_prepare(db->db, QUERY, &stmt))))
    goto done;

  // bind the where clause to our parameters
  if (ERROR((rc = sql_bind_int(stmt, 1, id))))
    goto done;
  if (ERROR((rc = sql_bind_int(stmt, 2, lineno))))
    goto done;

  // move to the first result
  const int r = sqlite3_step(stmt);
  if (r == SQLITE_DONE) {
    // no content
    rc = ENOMSG;
    goto done;
  } else if (ERROR((r != SQLITE_ROW))) {
    rc = sql_err_to_errno(r);
    goto done;
  }

  // duplicate it for the caller
  *content = strdup((const char *)sqlite3_column_text(stmt, 0));
  if (ERROR(*content == NULL)) {
    rc = ENOMEM;
    goto done;
  }

done:
  if (stmt != NULL)
    sqlite3_finalize(stmt);

  return rc;
}
