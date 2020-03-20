#include <clink/Error.h>
#include <sqlite3.h>
#include "SQLStatement.h"
#include <string>

namespace clink {

static int sql_prepare(sqlite3 *db, const char *query, sqlite3_stmt **stmt) {
  return sqlite3_prepare_v2(db, query, -1, stmt, nullptr);
}

static int sql_bind_text(sqlite3_stmt *stmt, int index, const char *value) {
  return sqlite3_bind_text(stmt, index, value, -1, SQLITE_STATIC);
}

static int sql_bind_int(sqlite3_stmt *stmt, int index, unsigned long value) {
  return sqlite3_bind_int64(stmt, index, sqlite3_int64(value));
}

SQLStatement::SQLStatement(sqlite3 *db, const char *query) {
  if (int rc = sql_prepare(db, query, &stmt))
    throw Error("failed to prepare SQL statement", rc);
}

void SQLStatement::bind(const char *param, int index, const std::string &value) {

  // confirm the parameter is aligned with the caller’s expectation
  assert(index == sqlite3_bind_parameter_index(stmt, param));
  (void)param;

  // bind the value to the given parameter
  if (int rc = sql_bind_text(stmt, index, value.c_str()))
    throw Error("failed to bind parameter to SQL statement", rc);
}

void SQLStatement::bind(const char *param, int index, unsigned long value) {

  // confirm the parameter is aligned with the caller’s expectation
  assert(index == sqlite3_bind_parameter_index(stmt, param));
  (void)param;

  // bind the value to the given parameter
  if (int rc = sql_bind_int(stmt, index, value))
    throw Error("failed to bind parameter to SQL statement", rc);
}

int SQLStatement::run() {
  return step();
}

int SQLStatement::step() {
  return sqlite3_step(stmt);
}

std::string SQLStatement::column_text(int index) const {
  return (const char*)sqlite3_column_text(stmt, index);
}

unsigned long SQLStatement::column_int(int index) const {
  return (unsigned long)sqlite3_column_int64(stmt, index);
}

SQLStatement::~SQLStatement() {
  sqlite3_finalize(stmt);
}

}
