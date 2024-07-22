#include "../../common/compiler.h"
#include "add_symbol.h"
#include "db.h"
#include "debug.h"
#include "get_id.h"
#include "span.h"
#include "sql.h"
#include <assert.h>
#include <clink/db.h>
#include <clink/symbol.h>
#include <errno.h>
#include <pthread.h>
#include <sqlite3.h>
#include <stddef.h>
#include <string.h>

static int add(sqlite3_stmt *stmt, clink_category_t category, span_t name,
               clink_record_id_t path, span_t parent) {

  assert(stmt != NULL);

  int rc = 0;

  assert(name.base != NULL);
  if (ERROR((rc = sql_bind_span(stmt, 1, name))))
    goto done;

  assert(path >= 0);
  if (ERROR((rc = sql_bind_int(stmt, 2, path))))
    goto done;

  if (ERROR((rc = sql_bind_int(stmt, 3, category))))
    goto done;

  if (ERROR((rc = sql_bind_int(stmt, 4, name.lineno))))
    goto done;

  if (ERROR((rc = sql_bind_int(stmt, 5, name.colno))))
    goto done;

  if (ERROR((rc = sql_bind_int(stmt, 6, name.start.lineno))))
    goto done;

  if (ERROR((rc = sql_bind_int(stmt, 7, name.start.colno))))
    goto done;

  if (ERROR((rc = sql_bind_int(stmt, 8, name.start.byte))))
    goto done;

  if (ERROR((rc = sql_bind_int(stmt, 9, name.end.lineno))))
    goto done;

  if (ERROR((rc = sql_bind_int(stmt, 10, name.end.colno))))
    goto done;

  if (ERROR((rc = sql_bind_int(stmt, 11, name.end.byte))))
    goto done;

  {
    if (parent.base == NULL)
      parent = (span_t){.base = "", .size = 0};
    if (ERROR((rc = sql_bind_span(stmt, 12, parent))))
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

int add_symbols(clink_db_t *db, size_t syms_size, symbol_t *syms,
                clink_record_id_t id) {

  assert(db != NULL);
  assert(syms_size == 0 || syms != NULL);
  assert(id >= 0);

  // insert into the symbol table

  static const char SYMBOL_INSERT[] =
      "insert or replace into symbols (name, path, category, line, col, "
      "start_line, start_col, start_byte, end_line, end_col, end_byte, parent) "
      "values (@name, @path, @category, @line, @col, @start_line, @start_col, "
      "@start_byte, @end_line, @end_col, @end_byte, @parent);";

  int rc = 0;

  // assume other `add_symbols` calls are being done concurrently and try to
  // serialise them to accelerate throughput
  if (ERROR((rc = pthread_mutex_lock(&db->bulk_operation))))
    return rc;

  sqlite3_stmt *s = NULL;
  if (ERROR((rc = sql_prepare(db->db, SYMBOL_INSERT, &s))))
    goto done;

  for (size_t i = 0; i < syms_size; ++i) {

    // if we already used the statement, clear it for reuse
    if (i > 0) {
      int r UNUSED = sqlite3_reset(s);
      assert(r == SQLITE_OK);
    }

#ifndef NDEBUG
    // if the caller passed a `path`, do them a favour and validate this matches
    // `id`
    if (syms[i].path != NULL) {
      clink_record_id_t cross_reference = -1;
      if (ERROR((rc = get_id(db, syms[i].path, &cross_reference))))
        goto done;
      assert(cross_reference == id);
    }
#endif

    if (ERROR(
            (rc = add(s, syms[i].category, syms[i].name, id, syms[i].parent))))
      goto done;
  }

done:
  if (s != NULL)
    sqlite3_finalize(s);

  {
    int r UNUSED = pthread_mutex_unlock(&db->bulk_operation);
    assert(r == 0);
  }

  return rc;
}

int add_symbol(clink_db_t *db, symbol_t sym) {

  assert(db != NULL);
  assert(sym.path != NULL);
  assert(sym.path[0] == '/');

  int rc = 0;

  // find the identifier for this symbol’s path
  clink_record_id_t id = -1;
  assert(sym.path != NULL);
  if (ERROR((rc = get_id(db, sym.path, &id))))
    goto done;

  rc = add_symbols(db, 1, &sym, id);

done:
  return rc;
}

int clink_db_add_symbol(clink_db_t *db, const clink_symbol_t *symbol) {

  if (ERROR(db == NULL))
    return EINVAL;

  if (ERROR(db->db == NULL))
    return EINVAL;

  if (ERROR(symbol == NULL))
    return EINVAL;

  if (ERROR(symbol->path == NULL || symbol->path[0] != '/'))
    return EINVAL;

  span_t name = {.base = symbol->name,
                 .lineno = symbol->lineno,
                 .colno = symbol->colno,
                 .start = symbol->start,
                 .end = symbol->end};
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
