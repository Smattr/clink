#pragma once

#include <cstddef>
#include <clink/Symbol.h>
#include <functional>
#include <sqlite3.h>
#include <string>

namespace clink {

// Clink symbol database
class Database {

 public:
  explicit Database(const std::string &path);

  /** add a symbol to the database
   *
   * \param s Symbol to add
   */
  void add(const Symbol &s);

  /** add a line of source content to the database
   *
   * \param path Path of the file this line came from
   * \param lineno Line number within the file this came from
   * \param line Content of the line itself
   */
  void add(const std::string &path, unsigned long lineno,
    const std::string &line);

  /** remove all symbols and content related to a given file
   *
   * \param path Path of the file to remove information for
   */
  void remove(const std::string &path);

  /** find a symbol in the database
   *
   * \param name Name of the symbol to lookup
   * \param callback Function to invoke for each found symbol
   * \returns 0 if run to completion, or the first non-zero returned by the
   *   callback if any
   */
  int find_symbol(const std::string &name,
    std::function<int(const Result&)> const &callback);

  /** find a definition in the database
   *
   * \param name Symbol name of the definition to lookup
   * \param callback Function to invoke for each found definition
   * \returns 0 if run to completion, or the first non-zero returned by the
   *   callback if any
   */
  int find_definition(const std::string &name,
    std::function<int(const Result&)> const &callback);

  /** find a functions that call a given function in the database
   *
   * \param name Symbol name of the function being called to lookup
   * \param callback Function to invoke for each found caller
   * \returns 0 if run to completion, or the first non-zero returned by the
   *   callback if any
   */
  int find_caller(const std::string &name,
    std::function<int(const Result&)> const &callback);

  /** find a function calls within a given function in the database
   *
   * \param name Symbol name of the containing function to lookup
   * \param callback Function to invoke for each found call
   * \returns 0 if run to completion, or the first non-zero returned by the
   *   callback if any
   */
  int find_call(const std::string &name,
    std::function<int(const Result&)> const &callback);

  /** find a given file in the database
   *
   * \param name Filename to lookup
   * \param callback Function to invoke for each found file
   * \returns 0 if run to completion, or the first non-zero returned by the
   *   callback if any
   */
  int find_file(const std::string &name,
    std::function<int(const std::string&)> const &callback);

  /** find #includes or a given file in the database
   *
   * \param name Filename of the function being #included
   * \param callback Function to invoke for each found #include
   * \returns 0 if run to completion, or the first non-zero returned by the
   *   callback if any
   */
  int find_includer(const std::string &name,
    std::function<int(const Result&)> const &callback);

  ~Database();

 private:
  /// handle to underlying database
  sqlite3 *db = nullptr;
};

}
