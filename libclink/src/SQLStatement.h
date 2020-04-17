#pragma once

#include <sqlite3.h>
#include <string>

namespace clink {

// RAII wrapper around sqlite3_stmt
class __attribute__((visibility("internal"))) SQLStatement {

 public:
  explicit SQLStatement(sqlite3 *db, const char *query);

  /** bind a value to a string parameter in the query
   *
   * \param param The name of the parameter (only used for a sanity assertion)
   * \param index The number of the parameter within the query
   * \param value The value to bind
   */
  void bind(const char *param, int index, const std::string &value);

  /** bind a value to an integer parameter in the query
   *
   * \param param The name of the parameter (only used for a sanity assertion)
   * \param index The number of the parameter within the query
   * \param value The value to bind
   */
  void bind(const char *param, int index, unsigned long value);

  /** run the underlying query
   *
   * \returns A SQLite error code
   */
  int run();

  /** step the underlying query forwards
   *
   * \returns A SQLite error code
   */
  int step();

  /** retrieve the value of a query column as text
   *
   * \param index Index of column to retrieve
   * \returns The value in the column at the current row
   */
  std::string column_text(int index) const;

  /** retrieve the value of a query column as an integer
   *
   * \param index Index of column to retrieve
   * \returns The value in the column at the current row
   */
  unsigned long column_int(int index) const;

  ~SQLStatement();

 private:
  sqlite3_stmt *stmt = nullptr;
};

}
