#include <assert.h>
#include <clink/db.h>
#include <clink/symbol.h>
#include "error.h"
#include "sql.h"
#include <sqlite3.h>
#include <stddef.h>
#include <string.h>

int clink_db_add_symbol(struct clink_db *db, const struct clink_symbol *s) {

  assert(db != NULL);
  assert(s != NULL);

  static const char INSERT[] = "insert into symbols (name, path, category, "
    "line, col, parent) values (@name, @path, @category, @line, @col, "
    "@parent);";

  int rc = 0;
  sqlite3_stmt *stmt = NULL;

  if ((rc = sql_prepare(db->handle, INSERT, &stmt))) {
    rc = sqlite_error(rc);
    goto done;
  }

  if ((rc = sql_bind_text(stmt, "@name", 1, s->name, s->name_len))) {
    rc = sqlite_error(rc);
    goto done;
  }

  if ((rc = sql_bind_text(stmt, "@path", 2, s->path, strlen(s->path)))) {
    rc = sqlite_error(rc);
    goto done;
  }

  if ((rc = sql_bind_int(stmt, "@category", 3, s->category))) {
    rc = sqlite_error(rc);
    goto done;
  }

  if ((rc = sql_bind_int(stmt, "@line", 4, s->lineno))) {
    rc = sqlite_error(rc);
    goto done;
  }

  if ((rc = sql_bind_int(stmt, "@col", 5, s->colno))) {
    rc = sqlite_error(rc);
    goto done;
  }

  const char *parent = s->parent == NULL ? "" : s->parent;
  if ((rc = sql_bind_text(stmt, "@parent", 6, parent, strlen(parent)))) {
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
