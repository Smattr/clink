#include <assert.h>
#include <clink/db.h>
#include <clink/symbol.h>
#include "db.h"
#include <errno.h>
#include "sql.h"
#include <sqlite3.h>
#include <stddef.h>

int clink_db_add_symbol(clink_db_t *db, const clink_symbol_t *symbol) {

  if (db == NULL)
    return EINVAL;

  if (db->db == NULL)
    return EINVAL;

  if (symbol == NULL)
    return EINVAL;

  // insert into the symbol table

  static const char SYMBOL_INSERT[] = "insert or replace into symbols (name, "
    "path, category, line, col, parent) values (@name, @path, @category, "
    "@line, @col, @parent);";

  int rc = 0;

  sqlite3_stmt *s = NULL;
  if ((rc = sql_prepare(db->db, SYMBOL_INSERT, &s)))
    goto done;

  assert(symbol->name != NULL);
  if ((rc = sql_bind_text(s, 1, symbol->name)))
    goto done;

  assert(symbol->path != NULL);
  if ((rc = sql_bind_text(s, 2, symbol->path)))
    goto done;

  if ((rc = sql_bind_int(s, 3, symbol->category)))
    goto done;

  if ((rc = sql_bind_int(s, 4, symbol->lineno)))
    goto done;

  if ((rc = sql_bind_int(s, 5, symbol->colno)))
    goto done;

  if ((rc = sql_bind_text(s, 6, symbol->parent == NULL ? "" : symbol->parent)))
    goto done;

  {
    int r = sqlite3_step(s);
    if (r != SQLITE_DONE) {
      rc = sql_err_to_errno(r);
      goto done;
    }
  }

done:
  if (s != NULL)
    sqlite3_finalize(s);

  return rc;
}
