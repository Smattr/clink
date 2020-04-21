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
