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
