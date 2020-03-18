#include <assert.h>
#include <clink/db.h>
#include "error.h"
#include "sql.h"
#include <sqlite3.h>
#include <stddef.h>
#include <string.h>

int clink_db_remove(struct clink_db *db, const char *path) {

  assert(db != NULL);
  assert(path != NULL);

  // first delete it from the symbols table

  static const char SYMBOLS_DELETE[] = "delete from symbols where path = @path";

  int rc = 0;
  sqlite3_stmt *symbols_delete = NULL;
  sqlite3_stmt *content_delete = NULL;

  if ((rc = sql_prepare(db->handle, SYMBOLS_DELETE, &symbols_delete))) {
    rc = sqlite_error(rc);
    goto done;
  }

  {
    int index = sqlite3_bind_parameter_index(symbols_delete, "@path");
    assert(index != 0);
    if ((rc = sql_bind_text(symbols_delete, index, path, strlen(path)))) {
      rc = sqlite_error(rc);
      goto done;
    }
  }

  {
    int result = sqlite3_step(symbols_delete);
    if (!sql_ok(result)) {
      rc = sqlite_error(result);
      goto done;
    }
  }

  // now delete it from the content table

  static const char CONTENT_DELETE[] = "delete from content where path = @path";

  if ((rc = sql_prepare(db->handle, CONTENT_DELETE, &content_delete))) {
    rc = sqlite_error(rc);
    goto done;
  }

  {
    int index = sqlite3_bind_parameter_index(content_delete, "@path");
    assert(index != 0);
    if ((rc = sql_bind_text(content_delete, index, path, strlen(path)))) {
      rc = sqlite_error(rc);
      goto done;
    }
  }

  {
    int result = sqlite3_step(content_delete);
    if (!sql_ok(result)) {
      rc = sqlite_error(result);
      goto done;
    }
  }

done:
  if (symbols_delete != NULL)
    sqlite3_finalize(symbols_delete);
  if (content_delete != NULL)
    sqlite3_finalize(content_delete);

  return rc;
}
