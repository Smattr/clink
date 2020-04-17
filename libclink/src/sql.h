#pragma once

#include <sqlite3.h>
#include <stdbool.h>
#include <stddef.h>

static inline int sql_exec(sqlite3 *db, const char *query) {
  return sqlite3_exec(db, query, NULL, NULL, NULL);
}

static inline bool sql_ok(int error) {
  return error == SQLITE_DONE || error == SQLITE_OK || error == SQLITE_ROW;
}

/// translate a SQLite primary result code to an errno
__attribute__((visibility("internal")))
int sql_err_to_errno(int err);
