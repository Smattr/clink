// some common utility wrappers around SQLite functionality

#pragma once

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

static inline int sql_bind_text(sqlite3_stmt *stmt, int index,
    const char *value, size_t value_len) {
  return sqlite3_bind_text(stmt, index, value, (int)value_len, SQLITE_STATIC);
}

static inline bool sql_ok(int error) {
  return error == SQLITE_DONE || error == SQLITE_OK || error == SQLITE_ROW;
}
