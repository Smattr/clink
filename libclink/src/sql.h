#pragma once

#include <sqlite3.h>
#include <stdbool.h>
#include <stddef.h>

static inline bool sql_ok(int error) {
  return error == SQLITE_DONE || error == SQLITE_OK || error == SQLITE_ROW;
}

/// translate a SQLite primary result code to an errno
__attribute__((visibility("internal")))
int sql_err_to_errno(int err);

static inline int sql_exec(sqlite3 *db, const char *query) {
  int r = sqlite3_exec(db, query, NULL, NULL, NULL);
  return sql_err_to_errno(r);
}

static inline int sql_prepare(sqlite3 *db, const char *query, sqlite3_stmt **stmt) {
  int r = sqlite3_prepare_v2(db, query, -1, stmt, NULL);
  return sql_err_to_errno(r);
}

static inline int sql_bind_text(sqlite3_stmt *stmt, int index, const char *value) {
  int r = sqlite3_bind_text(stmt, index, value, -1, SQLITE_STATIC);
  return sql_err_to_errno(r);
}

static inline int sql_bind_int(sqlite3_stmt *stmt, int index, unsigned long value) {
  int r =sqlite3_bind_int64(stmt, index, (sqlite3_int64)value);
  return sql_err_to_errno(r);
}
