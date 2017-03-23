#include <cassert>
#include <cstring>
#include "Database.h"
#include "errno.h"
#include "log.h"
#include <sqlite3.h>
#include <string>
#include "Symbol.h"
#include <unistd.h>
#include <vector>

using namespace std;

static int sql_exec(sqlite3 *db, const char *query) {
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
  "not null, col integer not null, parent text, "
  "unique(name, path, category, line, col));";

static const char CONTENT_SCHEMA[] = "create table if not exists content "
  "(path text not null, line integer not null, body text not null, "
  "unique(path, line));";

static int init(sqlite3 *db) {
  assert(db != nullptr);

  if (sql_exec(db, SYMBOLS_SCHEMA) != SQLITE_OK)
    return -1;

  if (sql_exec(db, CONTENT_SCHEMA) != SQLITE_OK)
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
  if (m_db != nullptr) {
    if (m_symbol_insert != nullptr)
      sqlite3_finalize(m_symbol_insert);
    if (m_content_insert != nullptr)
      sqlite3_finalize(m_content_insert);
    if (m_symbols_delete != nullptr)
      sqlite3_finalize(m_symbols_delete);
    if (m_content_delete != nullptr)
      sqlite3_finalize(m_content_delete);
    sqlite3_close(m_db);
    m_db = nullptr;
  }
}

bool Database::open_transaction() {
  assert(m_db != nullptr);
  return sql_exec(m_db, "begin transaction;") == SQLITE_OK;
}

bool Database::close_transaction() {
  assert(m_db != nullptr);
  return sql_exec(m_db, "commit transaction;") == SQLITE_OK;
}

void Database::consume(const Symbol &s) {
  assert(m_db != nullptr);

  // Insert into the symbol table.

  static const char SYMBOL_INSERT[] = "insert into symbols (name, path, "
    "category, line, col, parent) values (@name, @path, @category, @line, "
    "@col, @parent);";

  if (m_symbol_insert == nullptr) {
    if (sql_prepare(m_db, SYMBOL_INSERT, &m_symbol_insert) != SQLITE_OK)
      return;
  } else {
      if (sqlite3_reset(m_symbol_insert) != SQLITE_OK)
        return;
  }

  int index = 1;
  assert(index == sqlite3_bind_parameter_index(m_symbol_insert, "@name"));
  if (sql_bind_text(m_symbol_insert, index, s.name()) != SQLITE_OK)
    return;

  index = 2;
  assert(index == sqlite3_bind_parameter_index(m_symbol_insert, "@path"));
  if (sql_bind_text(m_symbol_insert, index, s.path()) != SQLITE_OK)
    return;

  index = 3;
  assert(index == sqlite3_bind_parameter_index(m_symbol_insert, "@category"));
  if (sqlite3_bind_int(m_symbol_insert, index, s.category()) != SQLITE_OK)
    return;

  index = 4;
  assert(index == sqlite3_bind_parameter_index(m_symbol_insert, "@line"));
  if (sqlite3_bind_int(m_symbol_insert, index, s.line()) != SQLITE_OK)
    return;

  index = 5;
  assert(index == sqlite3_bind_parameter_index(m_symbol_insert, "@col"));
  if (sqlite3_bind_int(m_symbol_insert, index, s.col()) != SQLITE_OK)
    return;

  index = 6;
  assert(index == sqlite3_bind_parameter_index(m_symbol_insert, "@parent"));
  if (sql_bind_text(m_symbol_insert, index, s.parent()) != SQLITE_OK)
    return;

  if (sqlite3_step(m_symbol_insert) != SQLITE_DONE)
    return;

  // Insert into the content table.

  static const char CONTENT_INSERT[] = "insert into content (path, line, body) "
    "values (@path, @line, @body);";

  if (m_content_insert == nullptr) {
    if (sql_prepare(m_db, CONTENT_INSERT, &m_content_insert) != SQLITE_OK)
      return;
  } else {
    if (sqlite3_reset(m_content_insert) != SQLITE_OK)
      return;
  }

  index = 1;
  assert(index == sqlite3_bind_parameter_index(m_content_insert, "@path"));
  if (sql_bind_text(m_content_insert, index, s.path()) != SQLITE_OK)
    return;

  index = 2;
  assert(index == sqlite3_bind_parameter_index(m_content_insert, "@line"));
  if (sqlite3_bind_int(m_content_insert, index, s.line()) != SQLITE_OK)
    return;

  index = 3;
  assert(index == sqlite3_bind_parameter_index(m_content_insert, "@body"));
  if (sql_bind_text(m_content_insert, index, s.context()) != SQLITE_OK)
    return;

  if (sqlite3_step(m_content_insert) != SQLITE_DONE)
    return;
}

bool Database::purge(const string &path) {

  // First delete it from the symbols table.

  static const char SYMBOLS_DELETE[] = "delete from symbols where path = @path";

  if (m_symbols_delete == nullptr) {
    if (sql_prepare(m_db, SYMBOLS_DELETE, &m_symbols_delete) != SQLITE_OK)
      return false;
  } else {
    if (sqlite3_reset(m_symbols_delete) != SQLITE_OK)
      return false;
  }

  int index = sqlite3_bind_parameter_index(m_symbols_delete, "@path");
  assert(index != 0);
  if (sql_bind_text(m_symbols_delete, index, path.c_str()) != SQLITE_OK)
    return false;

  if (sqlite3_step(m_symbols_delete) != SQLITE_DONE)
    return false;

  // Now delete it from the content table.

  static const char CONTENT_DELETE[] = "delete from content where path = @path";

  if (m_content_delete == nullptr) {
    if (sql_prepare(m_db, CONTENT_DELETE, &m_content_delete) != SQLITE_OK)
      return false;
  } else {
    if (sqlite3_reset(m_content_delete) != SQLITE_OK)
      return false;
  }

  index = sqlite3_bind_parameter_index(m_content_delete, "@path");
  assert(index != 0);
  if (sql_bind_text(m_content_delete, index, path.c_str()) != SQLITE_OK)
    return false;

  if (sqlite3_step(m_content_delete) != SQLITE_DONE)
    return false;

  return true;
}

vector<Symbol> Database::find_symbol(const char *name) const {
  assert(m_db != nullptr);

  vector<Symbol> vs;

  static const char QUERY[] = "select symbols.path, symbols.category, "
    "symbols.line, symbols.col, symbols.parent, content.body from symbols left "
    "join content on symbols.path = content.path and symbols.line = "
    "content.line where symbols.name = @name;";

  sqlite3_stmt *stmt = nullptr;
  if (sql_prepare(m_db, QUERY, &stmt) != SQLITE_OK) {
    LOG("failed to prepare query \"%s\"\n", QUERY);
    goto done;
  }

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

  static const char QUERY[] = "select symbols.path, symbols.line, symbols.col, "
    "symbols.parent, content.body from symbols left join content on "
    "symbols.path = content.path and symbols.line = content.line where "
    "symbols.name = @name and symbols.category = @category;";

  sqlite3_stmt *stmt = nullptr;
  if (sql_prepare(m_db, QUERY, &stmt) != SQLITE_OK) {
    LOG("failed to prepare query \"%s\"", QUERY);
    goto done;
  }

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

  static const char QUERY[] = "select symbols.path, symbols.line, symbols.col, "
    "symbols.parent, content.body from symbols left join content on "
    "symbols.path = content.path and symbols.line = content.line where "
    "symbols.name = @name and symbols.category = @category;";

  sqlite3_stmt *stmt = nullptr;
  if (sql_prepare(m_db, QUERY, &stmt) != SQLITE_OK) {
    LOG("failed to prepare query \"%s\"", QUERY);
    goto done;
  }

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

  static const char QUERY[] = "select symbols.name, symbols.path, "
    "symbols.line, symbols.col, content.body from symbols left join content on "
    "symbols.path = content.path and symbols.line = content.line where "
    "symbols.parent = @parent and symbols.category = @category;";

  sqlite3_stmt *stmt = nullptr;
  if (sql_prepare(m_db, QUERY, &stmt) != SQLITE_OK) {
    LOG("failed to prepare query \"%s\"", QUERY);
    goto done;
  }

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

  static const char QUERY[] = "select symbols.path, symbols.line, symbols.col, "
    "symbols.parent, content.body from symbols left join content on "
    "symbols.path = content.path and symbols.line = content.line where "
    "symbols.name = @name and symbols.category = @category;";

  sqlite3_stmt *stmt = nullptr;
  if (sql_prepare(m_db, QUERY, &stmt) != SQLITE_OK) {
    LOG("failed to prepare query \"%s\"", QUERY);
    goto done;
  }

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
