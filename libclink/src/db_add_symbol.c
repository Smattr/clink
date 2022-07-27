#include "add_symbol.h"
#include "db.h"
#include "debug.h"
#include "span.h"
#include "sql.h"
#include <assert.h>
#include <clink/db.h>
#include <clink/symbol.h>
#include <errno.h>
#include <sqlite3.h>
#include <stddef.h>
#include <string.h>

int add_symbol(clink_db_t *db, clink_category_t category, span_t name,
               const char *path, unsigned long lineno, unsigned long colno,
               span_t parent) {

  // insert into the symbol table

  static const char SYMBOL_INSERT[] =
      "insert or replace into symbols (name, "
      "path, category, line, col, parent) values (@name, @path, @category, "
      "@line, @col, @parent);";

  int rc = 0;

  sqlite3_stmt *s = NULL;
  if (ERROR((rc = sql_prepare(db->db, SYMBOL_INSERT, &s))))
    goto done;

  assert(name.base != NULL);
  if (ERROR((rc = sql_bind_span(s, 1, name))))
    goto done;

  assert(path != NULL);
  if (ERROR((rc = sql_bind_text(s, 2, path))))
    goto done;

  if (ERROR((rc = sql_bind_int(s, 3, category))))
    goto done;

  if (ERROR((rc = sql_bind_int(s, 4, lineno))))
    goto done;

  if (ERROR((rc = sql_bind_int(s, 5, colno))))
    goto done;

  {
    if (parent.base == NULL)
      parent = (span_t){.base = "", .size = 0};
    if (ERROR((rc = sql_bind_span(s, 6, parent))))
      goto done;
  }

  {
    int r = sqlite3_step(s);
    if (ERROR(r != SQLITE_DONE)) {
      rc = sql_err_to_errno(r);
      goto done;
    }
  }

done:
  if (s != NULL)
    sqlite3_finalize(s);

  return rc;
}

int clink_db_add_symbol(clink_db_t *db, const clink_symbol_t *symbol) {

  if (ERROR(db == NULL))
    return EINVAL;

  if (ERROR(db->db == NULL))
    return EINVAL;

  if (ERROR(symbol == NULL))
    return EINVAL;

  span_t name = {.base = symbol->name};
  if (symbol->name != NULL)
    name.size = strlen(symbol->name);

  span_t parent = {.base = symbol->parent};
  if (symbol->parent != NULL)
    parent.size = strlen(symbol->parent);

  return add_symbol(db, symbol->category, name, symbol->path, symbol->lineno,
                    symbol->colno, parent);
}
