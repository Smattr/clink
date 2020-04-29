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
