#include "../../common/compiler.h"
#include "db.h"
#include "sql.h"
#include <assert.h>
#include <clink/db.h>
#include <clink/symbol.h>
#include <errno.h>
#include <sqlite3.h>
#include <stddef.h>

int clink_db_add_symbol(clink_db_t *db, const clink_symbol_t *symbol) {

  if (UNLIKELY(db == NULL))
    return EINVAL;

  if (UNLIKELY(db->db == NULL))
    return EINVAL;

  if (UNLIKELY(symbol == NULL))
    return EINVAL;

  // insert into the symbol table

  static const char SYMBOL_INSERT[] =
      "insert or replace into symbols (name, "
      "path, category, line, col, parent) values (@name, @path, @category, "
      "@line, @col, @parent);";

  int rc = 0;

  sqlite3_stmt *s = NULL;
  if (UNLIKELY((rc = sql_prepare(db->db, SYMBOL_INSERT, &s))))
    goto done;

  assert(symbol->name != NULL);
  if (UNLIKELY((rc = sql_bind_text(s, 1, symbol->name))))
    goto done;

  assert(symbol->path != NULL);
  if (UNLIKELY((rc = sql_bind_text(s, 2, symbol->path))))
    goto done;

  if (UNLIKELY((rc = sql_bind_int(s, 3, symbol->category))))
    goto done;

  if (UNLIKELY((rc = sql_bind_int(s, 4, symbol->lineno))))
    goto done;

  if (UNLIKELY((rc = sql_bind_int(s, 5, symbol->colno))))
    goto done;

  {
    const char *parent = symbol->parent == NULL ? "" : symbol->parent;
    if (UNLIKELY((rc = sql_bind_text(s, 6, parent))))
      goto done;
  }

  {
    int r = sqlite3_step(s);
    if (UNLIKELY(r != SQLITE_DONE)) {
      rc = sql_err_to_errno(r);
      goto done;
    }
  }

done:
  if (s != NULL)
    sqlite3_finalize(s);

  return rc;
}
