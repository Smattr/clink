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

static int add(sqlite3_stmt *stmt, clink_category_t category, span_t name,
               const char *path, span_t parent) {

  assert(stmt != NULL);

  int rc = 0;

  assert(name.base != NULL);
  if (ERROR((rc = sql_bind_span(stmt, 1, name))))
    goto done;

  assert(path != NULL);
  if (ERROR((rc = sql_bind_text(stmt, 2, path))))
    goto done;

  if (ERROR((rc = sql_bind_int(stmt, 3, category))))
    goto done;

  if (ERROR((rc = sql_bind_int(stmt, 4, name.lineno))))
    goto done;

  if (ERROR((rc = sql_bind_int(stmt, 5, name.colno))))
    goto done;

  {
    if (parent.base == NULL)
      parent = (span_t){.base = "", .size = 0};
    if (ERROR((rc = sql_bind_span(stmt, 6, parent))))
      goto done;
  }

  {
    int r = sqlite3_step(stmt);
    if (ERROR(r != SQLITE_DONE)) {
      rc = sql_err_to_errno(r);
      goto done;
    }
  }

done:
  return rc;
}

int add_symbols(clink_db_t *db, size_t syms_size, symbol_t *syms) {

  assert(db != NULL);
  assert(syms_size == 0 || syms != NULL);

  // insert into the symbol table

  static const char SYMBOL_INSERT[] =
      "insert or replace into symbols (name, "
      "path, category, line, col, parent) values (@name, @path, @category, "
      "@line, @col, @parent);";

  int rc = 0;

  sqlite3_stmt *s = NULL;
  if (ERROR((rc = sql_prepare(db->db, SYMBOL_INSERT, &s))))
    goto done;

  for (size_t i = 0; i < syms_size; ++i) {

    // if we already used the statement, clear it for reuse
    if (i > 0) {
      int r = sqlite3_reset(s);
      assert(r == SQLITE_OK);
      if (ERROR((r = sqlite3_clear_bindings(s)))) {
        rc = sql_err_to_errno(r);
        goto done;
      }
    }

    if (ERROR((rc = add(s, syms[i].category, syms[i].name, syms[i].path,
                        syms[i].parent))))
      goto done;
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

  span_t name = {
      .base = symbol->name, .lineno = symbol->lineno, .colno = symbol->colno};
  if (symbol->name != NULL)
    name.size = strlen(symbol->name);

  span_t parent = {.base = symbol->parent};
  if (symbol->parent != NULL)
    parent.size = strlen(symbol->parent);

  symbol_t sym = {.category = symbol->category,
                  .name = name,
                  .path = symbol->path,
                  .parent = parent};

  return add_symbol(db, sym);
}
