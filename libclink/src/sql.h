// some common utility wrappers around SQLite functionality

#pragma once

#include <assert.h>
#include <sqlite3.h>
#include <stdbool.h>
#include <stddef.h>

static inline int sql_exec(sqlite3 *db, const char *query) {
  return sqlite3_exec(db, query, NULL, NULL, NULL);
}

static inline int sql_prepare(sqlite3 *db, const char *query,
    sqlite3_stmt **stmt) {
  return sqlite3_prepare_v2(db, query, -1, stmt, NULL);
}

static inline int sql_bind_text(sqlite3_stmt *stmt, const char *param,
    int index, const char *value, size_t value_len) {

  // sanity check that the index is correct
  assert(index == sqlite3_bind_parameter_index(stmt, param)
    && "incorrect parameter index");
  (void)param;

  return sqlite3_bind_text(stmt, index, value, (int)value_len, SQLITE_STATIC);
}

static inline int sql_bind_int(sqlite3_stmt *stmt, const char *param,
    int index, int value) {

  // sanity check that the index is correct
  assert(index == sqlite3_bind_parameter_index(stmt, param)
    && "incorrect parameter index");
  (void)param;

  return sqlite3_bind_int(stmt, index, value);
}

static inline bool sql_ok(int error) {
  return error == SQLITE_DONE || error == SQLITE_OK || error == SQLITE_ROW;
}
