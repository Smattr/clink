#pragma once

#include <cstddef>
#include <clink/Symbol.h>
#include <functional>
#include <sqlite3.h>
#include <string>
#include <vector>

namespace clink {

// Clink symbol database
class Database {

 public:
  explicit Database(const std::string &path);

  /** find a symbol in the database
   *
   * \param name Name of the symbol to lookup
   * \param callback Function to invoke for each found symbol
   * \returns 0 if run to completion, or the first non-zero returned by the
   *   callback if any
   */
  int find_symbol(const std::string &name,
    std::function<int(const Result&)> const &callback);

  /** equivalent of find_symbol, but returning all results in a collection
   *
   * \param name Name of the symbol to lookup
   * \returns all symbols found
   */
  std::vector<Result> find_symbols(const std::string &name);

  /** find a definition in the database
   *
   * \param name Symbol name of the definition to lookup
   * \param callback Function to invoke for each found definition
   * \returns 0 if run to completion, or the first non-zero returned by the
   *   callback if any
   */
  int find_definition(const std::string &name,
    std::function<int(const Result&)> const &callback);

  /** equivalent of find_definition, but returning all results in a collection
   *
   * \param name Symbol name of the definition to lookup
   * \returns all definitions found
   */
  std::vector<Result> find_definitions(const std::string &name);

  /** find a functions that call a given function in the database
   *
   * \param name Symbol name of the function being called to lookup
   * \param callback Function to invoke for each found caller
   * \returns 0 if run to completion, or the first non-zero returned by the
   *   callback if any
   */
  int find_caller(const std::string &name,
    std::function<int(const Result&)> const &callback);

  /** equivalent of find_caller, but returning all results in a collection
   *
   * \param name Symbol name of the function being called to lookup
   * \returns all callers found
   */
  std::vector<Result> find_callers(const std::string &name);

  /** find a function calls within a given function in the database
   *
   * \param name Symbol name of the containing function to lookup
   * \param callback Function to invoke for each found call
   * \returns 0 if run to completion, or the first non-zero returned by the
   *   callback if any
   */
  int find_call(const std::string &name,
    std::function<int(const Result&)> const &callback);

  /** equivalent of find_call, but returning all results in a collection
   *
   * \param name Symbol name of the containing function to lookup
   * \returns all calls found
   */
  std::vector<Result> find_calls(const std::string &name);

  /** find a given file in the database
   *
   * \param name Filename to lookup
   * \param callback Function to invoke for each found file
   * \returns 0 if run to completion, or the first non-zero returned by the
   *   callback if any
   */
  int find_file(const std::string &name,
    std::function<int(const std::string&)> const &callback);

  /** equivalent of find_file, but returning all results in a collection
   *
   * \param name Filename to lookup
   * \returns all files found
   */
  std::vector<std::string> find_files(const std::string &name);

  /** find #includes or a given file in the database
   *
   * \param name Filename of the function being #included
   * \param callback Function to invoke for each found #include
   * \returns 0 if run to completion, or the first non-zero returned by the
   *   callback if any
   */
  int find_includer(const std::string &name,
    std::function<int(const Result&)> const &callback);

  /** equivalent of fine_includer, but returning all results in a collection
   *
   * \param name Filename of the function being #included
   * \returns all includers found
   */
  std::vector<Result> find_includers(const std::string &name);

  ~Database();

 private:
  /// handle to underlying database
  sqlite3 *db = nullptr;
};

}
