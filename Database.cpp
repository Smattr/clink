#include <cassert>
#include <cstring>
#include "Database.h"
#include "errno.h"
#include <sqlite3.h>
#include <string>
#include "Symbol.h"
#include <unistd.h>
#include <vector>

using namespace std;

static int sql_run(sqlite3 *db, const char *query) {
  return sqlite3_exec(db, query, nullptr, nullptr, nullptr);
}

static int sql_prepare(sqlite3 *db, const char *query, sqlite3_stmt **stmt) {
  return sqlite3_prepare_v2(db, query, -1, stmt, nullptr);
}

static int sql_bind_text(sqlite3_stmt *stmt, int index, const char *value) {
  return sqlite3_bind_text(stmt, index, value, -1, SQLITE_STATIC);
}

static const char SYMBOLS_SCHEMA[] = "create table if not exists symbols (name "
  "text not null, path text not null, category integer not null, line integer "
  "not null, col integer not null, parent text, context text, "
  "unique(name, path, category, line, col));";

static int init(sqlite3 *db) {
  assert(db != nullptr);

  if (sqlite3_exec(db, SYMBOLS_SCHEMA, nullptr, nullptr, nullptr)
      != SQLITE_OK)
    return -1;

  return 0;
}

bool Database::open(const char *path) {

  /* Fail if we already have a database open. */
  if (m_db)
    return false;

  /* Check if the file exists, so we know whether to create the database
   * structure.
   */
  bool exists = !(access(path, R_OK|W_OK) == -1 && errno == ENOENT);

  if (sqlite3_open(path, &m_db) != SQLITE_OK)
    return false;

  if (!exists) {
    if (init(m_db) != 0)
      return false;
  }

  return true;
}

void Database::close() {
  if (m_db) {
    if (m_insert)
      sqlite3_finalize(m_insert);
    sqlite3_close(m_db);
    m_db = nullptr;
  }
}

bool Database::open_transaction() {
  assert(m_db != nullptr);
  return sql_run(m_db, "begin transaction;") == SQLITE_OK;
}

bool Database::close_transaction() {
  assert(m_db != nullptr);
  return sql_run(m_db, "commit transaction;") == SQLITE_OK;
}

void Database::consume(const Symbol &s) {
  assert(m_db != nullptr);

  if (m_insert == nullptr) {
    if (sql_prepare(m_db, "insert into symbols (name, path, "
        "category, line, col, parent, context) values (@name, @path, "
        "@category, @line, @col, @parent, @context);", &m_insert) != SQLITE_OK)
      return;
  } else {
      if (sqlite3_reset(m_insert) != SQLITE_OK)
        return;
  }

  int index = 1;
  assert(index == sqlite3_bind_parameter_index(m_insert, "@name"));
  if (sql_bind_text(m_insert, index, s.name()) != SQLITE_OK)
    return;

  index = 2;
  assert(index == sqlite3_bind_parameter_index(m_insert, "@path"));
  if (sql_bind_text(m_insert, index, s.path()) != SQLITE_OK)
    return;

  index = 3;
  assert(index == sqlite3_bind_parameter_index(m_insert, "@category"));
  if (sqlite3_bind_int(m_insert, index, s.category()) != SQLITE_OK)
    return;

  index = 4;
  assert(index == sqlite3_bind_parameter_index(m_insert, "@line"));
  if (sqlite3_bind_int(m_insert, index, s.line()) != SQLITE_OK)
    return;

  index = 5;
  assert(index == sqlite3_bind_parameter_index(m_insert, "@col"));
  if (sqlite3_bind_int(m_insert, index, s.col()) != SQLITE_OK)
    return;

  index = 6;
  assert(index == sqlite3_bind_parameter_index(m_insert, "@parent"));
  if (sql_bind_text(m_insert, index, s.parent()) != SQLITE_OK)
    return;

  index = 7;
  assert(index == sqlite3_bind_parameter_index(m_insert, "@context"));
  if (sql_bind_text(m_insert, index, s.context()) != SQLITE_OK)
    return;

  if (sqlite3_step(m_insert) != SQLITE_DONE)
    return;
}

bool Database::purge(const string &path) {

  if (m_delete == nullptr) {
    if (sql_prepare(m_db, "delete from symbols where path = @path;", &m_delete)
        != SQLITE_OK)
      return false;
  } else {
    if (sqlite3_reset(m_delete) != SQLITE_OK)
      return false;
  }

  int index = sqlite3_bind_parameter_index(m_delete, "@path");
  assert(index != 0);
  if (sql_bind_text(m_delete, index, path.c_str()) != SQLITE_OK)
    return false;

  if (sqlite3_step(m_delete) != SQLITE_DONE)
    return false;

  return true;
}

vector<Symbol> Database::find_symbol(const char *name) const {
  assert(m_db != nullptr);

  vector<Symbol> vs;

  sqlite3_stmt *stmt = nullptr;
  if (sql_prepare(m_db, "select path, category, line, col, parent, "
      "context from symbols where name = @name;", &stmt) != SQLITE_OK)
    goto done;

  if (sql_bind_text(stmt, 1, name) != SQLITE_OK)
    goto done;

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    const char *path = (char*)sqlite3_column_text(stmt, 0);
    symbol_category_t cat = symbol_category_t(sqlite3_column_int(stmt, 1));
    unsigned line = unsigned(sqlite3_column_int(stmt, 2));
    unsigned col = unsigned(sqlite3_column_int(stmt, 3));
    const char *parent = (char*)sqlite3_column_text(stmt, 4);
    const char *context = (char*)sqlite3_column_text(stmt, 5);

    Symbol s(name, path, cat, line, col, parent, context);
    vs.push_back(s);
  }

done:
  if (stmt != nullptr)
    sqlite3_finalize(stmt);
  return vs;
}

vector<Symbol> Database::find_definition(const char *name) const {
  assert(m_db != nullptr);

  vector<Symbol> vs;

  sqlite3_stmt *stmt = nullptr;
  if (sql_prepare(m_db, "select path, line, col, parent, context from "
      "symbols where name = @name and category = @category;", &stmt)
      != SQLITE_OK)
    goto done;

  if (sql_bind_text(stmt, 1, name) != SQLITE_OK)
    goto done;

  if (sqlite3_bind_int(stmt, 2, ST_DEFINITION) != SQLITE_OK)
    goto done;

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    const char *path = (char*)sqlite3_column_text(stmt, 0);
    unsigned line = unsigned(sqlite3_column_int(stmt, 1));
    unsigned col = unsigned(sqlite3_column_int(stmt, 2));
    const char *parent = (char*)sqlite3_column_text(stmt, 3);
    const char *context = (char*)sqlite3_column_text(stmt, 4);

    Symbol s(name, path, ST_DEFINITION, line, col, parent, context);
    vs.push_back(s);
  }

done:
  if (stmt != nullptr)
    sqlite3_finalize(stmt);
  return vs;
}

vector<Symbol> Database::find_caller(const char *name) const {
  assert(m_db != nullptr);

  vector<Symbol> vs;

  sqlite3_stmt *stmt = nullptr;
  if (sql_prepare(m_db, "select path, line, col, parent, context from "
      "symbols where name = @name and category = @category;", &stmt)
      != SQLITE_OK)
    goto done;

  if (sql_bind_text(stmt, 1, name) != SQLITE_OK)
    goto done;

  if (sqlite3_bind_int(stmt, 2, ST_FUNCTION_CALL) != SQLITE_OK)
    goto done;

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    const char *path = (char*)sqlite3_column_text(stmt, 0);
    unsigned line = unsigned(sqlite3_column_int(stmt, 1));
    unsigned col = unsigned(sqlite3_column_int(stmt, 2));
    const char *parent = (char*)sqlite3_column_text(stmt, 3);
    const char *context = (char*)sqlite3_column_text(stmt, 4);

    Symbol s(name, path, ST_FUNCTION_CALL, line, col, parent, context);
    vs.push_back(s);
  }

done:
  if (stmt != nullptr)
    sqlite3_finalize(stmt);
  return vs;
}

vector<Symbol> Database::find_call(const char *name) const {
  assert(m_db != nullptr);

  vector<Symbol> vs;

  sqlite3_stmt *stmt = nullptr;
  if (sql_prepare(m_db, "select name, path, line, col, context from "
      "symbols where parent = @parent and category = @category;", &stmt)
      != SQLITE_OK)
    goto done;

  if (sql_bind_text(stmt, 1, name) != SQLITE_OK)
    goto done;

  if (sqlite3_bind_int(stmt, 2, ST_FUNCTION_CALL) != SQLITE_OK)
    goto done;

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    const char *call = (char*)sqlite3_column_text(stmt, 0);
    const char *path = (char*)sqlite3_column_text(stmt, 1);
    unsigned line = unsigned(sqlite3_column_int(stmt, 2));
    unsigned col = unsigned(sqlite3_column_int(stmt, 3));
    const char *context = (char*)sqlite3_column_text(stmt, 4);

    Symbol s(call, path, ST_FUNCTION_CALL, line, col, name, context);
    vs.push_back(s);
  }

done:
  if (stmt != nullptr)
    sqlite3_finalize(stmt);
  return vs;
}

vector<string> Database::find_file(const char *name) const {
  assert(m_db != nullptr);

  vector<string> vs;

  string path2("%/");
  path2 += name;

  sqlite3_stmt *stmt = nullptr;
  if (sql_prepare(m_db, "select distinct path from symbols where path = "
      "@path1 or path like @path2;", &stmt) != SQLITE_OK)
    goto done;

  if (sql_bind_text(stmt, 1, name) != SQLITE_OK)
    goto done;

  if (sql_bind_text(stmt, 2, path2.c_str()) != SQLITE_OK)
    goto done;

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    const char *path = (char*)sqlite3_column_text(stmt, 0);
    string s(path);
    vs.push_back(s);
  }

done:
  if (stmt != nullptr)
    sqlite3_finalize(stmt);
  return vs;
}

vector<Symbol> Database::find_includer(const char *name) const {
  assert(m_db != nullptr);

  vector<Symbol> vs;

  sqlite3_stmt *stmt = nullptr;
  if (sql_prepare(m_db, "select path, line, col, parent, context from "
      "symbols where name = @name and category = @category;", &stmt)
      != SQLITE_OK)
    goto done;

  if (sql_bind_text(stmt, 1, name) != SQLITE_OK)
    goto done;

  if (sqlite3_bind_int(stmt, 2, ST_INCLUDE) != SQLITE_OK)
    goto done;

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    const char *path = (char*)sqlite3_column_text(stmt, 0);
    unsigned line = unsigned(sqlite3_column_int(stmt, 1));
    unsigned col = unsigned(sqlite3_column_int(stmt, 2));
    const char *parent = (char*)sqlite3_column_text(stmt, 3);
    const char *context = (char*)sqlite3_column_text(stmt, 4);

    Symbol s(name, path, ST_INCLUDE, line, col, parent, context);
    vs.push_back(s);
  }

done:
  if (stmt != nullptr)
    sqlite3_finalize(stmt);
  return vs;
}

Database::~Database() {
  if (m_db)
    close();
}
