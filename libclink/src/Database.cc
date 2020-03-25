#include <cassert>
#include <clink/Database.h>
#include <clink/Error.h>
#include <clink/Symbol.h>
#include <errno.h>
#include <functional>
#include <sqlite3.h>
#include "SQLStatement.h"
#include <string>
#include <unistd.h>
#include <vector>

namespace clink {

static const char SYMBOLS_SCHEMA[] = "create table if not exists symbols (name "
  "text not null, path text not null, category integer not null, line integer "
  "not null, col integer not null, parent text, "
  "unique(name, path, category, line, col));";

static const char CONTENT_SCHEMA[] = "create table if not exists content "
  "(path text not null, line integer not null, body text not null, "
  "unique(path, line));";

static const char *PRAGMAS[] = {
  "pragma synchronous=OFF;",
  "pragma journal_mode=MEMORY;",
  "pragma temp_store=MEMORY;",
};

static void init(sqlite3 *db) {
  assert(db != nullptr);

  if (int rc = sql_exec(db, SYMBOLS_SCHEMA))
    throw Error("failed to create database symbol table", rc);

  if (int rc = sql_exec(db, CONTENT_SCHEMA))
    throw Error("failed to create database content table", rc);

  for (const char *pragma : PRAGMAS)
    if (int rc = sql_exec(db, pragma))
      throw Error("failed to run database pragma", rc);
}


Database::Database(const std::string &path) {

  // check if the database file already exists, so we know whether to create the
  // database structure
  bool exists = !(access(path.c_str(), R_OK|W_OK) == -1 && errno == ENOENT);

  int rc = sqlite3_open(path.c_str(), &db);
  if (rc != SQLITE_OK)
    throw Error("failed to open database", rc);

  if (!exists)
    init(db);
}

void Database::add(const Symbol &s) {
  assert(db != nullptr);

  // insert into the symbol table

  static const char SYMBOL_INSERT[] = "insert into symbols (name, path, "
    "category, line, col, parent) values (@name, @path, @category, @line, "
    "@col, @parent);";

  SQLStatement stmt(db, SYMBOL_INSERT);

  stmt.bind("@name", 1, s.name);
  stmt.bind("@path", 2, s.path);
  stmt.bind("@category", 3, s.category);
  stmt.bind("@line", 4, s.lineno);
  stmt.bind("@col", 5, s.colno);
  stmt.bind("@parent", 6, s.parent);

  if (int rc = stmt.run())
    throw Error("symbol insertion failed", rc);
}

void Database::add(const std::string &path, unsigned long lineno,
    const std::string &line) {
  assert(db != nullptr);

  // insert into the content table

  static const char CONTENT_INSERT[] = "insert into content (path, line, body) "
    "values (@path, @line, @body);";

  SQLStatement stmt(db, CONTENT_INSERT);

  stmt.bind("@path", 1, path);
  stmt.bind("@line", 2, lineno);
  stmt.bind("@body", 3, line);

  if (int rc = stmt.run())
    throw Error("content insertion failed", rc);
}

void Database::remove(const std::string &path) {

  // first delete it from the symbols table
  {
    static const char SYMBOLS_DELETE[] = "delete from symbols where path = @path";
    SQLStatement stmt(db, SYMBOLS_DELETE);

    stmt.bind("@path", 1, path);

    int rc = stmt.run();
    if (!sql_ok(rc))
      throw Error("failed to delete path from symbols table", rc);
  }

  // now delete it from the content table.
  {
    static const char CONTENT_DELETE[] = "delete from content where path = @path";
    SQLStatement stmt(db, CONTENT_DELETE);

    stmt.bind("@path", 1, path);

    int rc = stmt.run();
    if (!sql_ok(rc))
      throw Error("failed to delete path from content table", rc);
  }
}

int Database::find_symbol(const std::string &name,
    std::function<int(const Result&)> const &callback) {

  static const char QUERY[] = "select symbols.path, symbols.category, "
    "symbols.line, symbols.col, symbols.parent, content.body from symbols left "
    "join content on symbols.path = content.path and symbols.line = "
    "content.line where symbols.name = @name;";

  SQLStatement stmt(db, QUERY);

  stmt.bind("@name", 1, name);

  while (stmt.step() == SQLITE_ROW) {

    const std::string path    = stmt.column_text(0);
    Symbol::Category cat      = static_cast<Symbol::Category>(stmt.column_int(1));
    unsigned long lineno      = stmt.column_int(2);
    unsigned long colno       = stmt.column_int(3);
    const std::string parent  = stmt.column_text(4);
    const std::string context = stmt.column_text(5);

    Result r{Symbol{cat, name, path, lineno, colno, parent}, context};
    if (int rc = callback(r))
      return rc;
  }

  return 0;
}

std::vector<Result> Database::find_symbols(const std::string &name) {
  std::vector<Result> rs;
  (void)find_symbol(name, [&](const Result &r) {
    rs.push_back(r);
    return 0;
  });
  return rs;
}

int Database::find_definition(const std::string &name,
    std::function<int(const Result&)> const &callback) {
  assert(db != nullptr);

  static const char QUERY[] = "select symbols.path, symbols.line, symbols.col, "
    "symbols.parent, content.body from symbols left join content on "
    "symbols.path = content.path and symbols.line = content.line where "
    "symbols.name = @name and symbols.category = @category;";

  SQLStatement stmt(db, QUERY);

  stmt.bind("@name", 1, name);
  stmt.bind("@category", 2, Symbol::DEFINITION);

  while (stmt.step() == SQLITE_ROW) {

    const std::string path    = stmt.column_text(0);
    Symbol::Category cat      = Symbol::DEFINITION;
    unsigned long lineno      = stmt.column_int(1);
    unsigned long colno       = stmt.column_int(2);
    const std::string parent  = stmt.column_text(3);
    const std::string context = stmt.column_text(4);

    Result r{Symbol{cat, name, path, lineno, colno, parent}, context};
    if (int rc = callback(r))
      return rc;
  }

  return 0;
}

std::vector<Result> Database::find_definitions(const std::string &name) {
  std::vector<Result> rs;
  (void)find_definition(name, [&](const Result &r) {
    rs.push_back(r);
    return 0;
  });
  return rs;
}

int Database::find_caller(const std::string &name,
    std::function<int(const Result&)> const &callback) {
  assert(db != nullptr);

  static const char QUERY[] = "select symbols.path, symbols.line, symbols.col, "
    "symbols.parent, content.body from symbols left join content on "
    "symbols.path = content.path and symbols.line = content.line where "
    "symbols.name = @name and symbols.category = @category;";

  SQLStatement stmt(db, QUERY);

  stmt.bind("@name", 1, name);
  stmt.bind("@category", 2, Symbol::FUNCTION_CALL);

  while (stmt.step() == SQLITE_ROW) {

    const std::string path    = stmt.column_text(0);
    Symbol::Category cat      = Symbol::FUNCTION_CALL;
    unsigned long lineno      = stmt.column_int(1);
    unsigned long colno       = stmt.column_int(2);
    const std::string parent  = stmt.column_text(3);
    const std::string context = stmt.column_text(4);

    Result r{Symbol{cat, name, path, lineno, colno, parent}, context};
    if (int rc = callback(r))
      return rc;
  }

  return 0;
}

std::vector<Result> Database::find_callers(const std::string &name) {
  std::vector<Result> rs;
  (void)find_caller(name, [&](const Result &r) {
    rs.push_back(r);
    return 0;
  });
  return rs;
}

int Database::find_call(const std::string &name,
    std::function<int(const Result&)> const &callback) {
  assert(db != nullptr);

  static const char QUERY[] = "select symbols.name, symbols.path, "
    "symbols.line, symbols.col, content.body from symbols left join content on "
    "symbols.path = content.path and symbols.line = content.line where "
    "symbols.parent = @parent and symbols.category = @category;";

  SQLStatement stmt(db, QUERY);

  stmt.bind("@parent", 1, name);
  stmt.bind("@category", 2, Symbol::FUNCTION_CALL);

  while (stmt.step() == SQLITE_ROW) {

    const std::string call    = stmt.column_text(0);
    const std::string path    = stmt.column_text(1);
    Symbol::Category cat      = Symbol::FUNCTION_CALL;
    unsigned long lineno      = stmt.column_int(2);
    unsigned long colno       = stmt.column_int(3);
    const std::string context = stmt.column_text(4);

    Result r{Symbol{cat, call, path, lineno, colno, name}, context};
    if (int rc = callback(r))
      return rc;
  }

  return 0;
}

std::vector<Result> Database::find_calls(const std::string &name) {
  std::vector<Result> rs;
  (void)find_call(name, [&](const Result &r) {
    rs.push_back(r);
    return 0;
  });
  return rs;
}

int Database::find_file(const std::string &name,
    std::function<int(const std::string&)> const &callback) {
  assert(db != nullptr);

  std::string path2("%/");
  path2 += name;

  static const char QUERY[] = "select distinct path from symbols where path = "
    "@path1 or path like @path2;";

  SQLStatement stmt(db, QUERY);

  stmt.bind("@path1", 1, name);
  stmt.bind("@path2", 2, path2);

  while (stmt.step() == SQLITE_ROW) {
    const std::string path = stmt.column_text(0);
    if (int rc = callback(path))
      return rc;
  }

  return 0;
}

std::vector<std::string> Database::find_files(const std::string &name) {
  std::vector<std::string> rs;
  (void)find_file(name, [&](const std::string &r) {
    rs.push_back(r);
    return 0;
  });
  return rs;
}

int Database::find_includer(const std::string &name,
    std::function<int(const Result&)> const &callback) {
  assert(db != nullptr);

  static const char QUERY[] = "select symbols.path, symbols.line, symbols.col, "
    "symbols.parent, content.body from symbols left join content on "
    "symbols.path = content.path and symbols.line = content.line where "
    "symbols.name = @name and symbols.category = @category;";

  SQLStatement stmt(db, QUERY);

  stmt.bind("@name", 1, name);
  stmt.bind("@category", 2, Symbol::INCLUDE);

  while (stmt.step() == SQLITE_ROW) {

    const std::string path    = stmt.column_text(0);
    Symbol::Category cat      = Symbol::INCLUDE;
    unsigned long lineno      = stmt.column_int(1);
    unsigned long colno       = stmt.column_int(2);
    const std::string parent  = stmt.column_text(3);
    const std::string context = stmt.column_text(4);

    Result r{Symbol{cat, name, path, lineno, colno, parent}, context};
    if (int rc = callback(r))
      return rc;
  }

  return 0;
}

std::vector<Result> Database::find_includers(const std::string &name) {
  std::vector<Result> rs;
  (void)find_includer(name, [&](const Result &r) {
    rs.push_back(r);
    return 0;
  });
  return rs;
}

Database::~Database() {
  sqlite3_close(db);
}

}
