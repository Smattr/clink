#pragma once

#include <sqlite3.h>
#include <string>
#include "Symbol.h"
#include <vector>

class Database;

/* An object for tracking a list of actions to be applied to a Database.
 *
 * The idea is that, since actions are not expected to conflict cross-file, we
 * can run multiple threads accumulating per-thread PendingActions, and then
 * serially replay them into a single Database.
 */
class PendingActions : public SymbolConsumer {

 public:
  void consume(const Symbol &s) final {
    symbols.push_back(s);
  }
  bool purge(const std::string &path) final {
    to_purge.push_back(path);
    return true;
  }

  virtual ~PendingActions() {}

 private:
  std::vector<Symbol> symbols;
  std::vector<std::string> to_purge;

  friend class Database;
};

class Database : public SymbolConsumer {

 public:
  virtual ~Database();
  bool open(const char *path);
  void close();
  void consume(const Symbol &s) final;
  bool purge(const std::string &path) final;
  bool open_transaction();
  bool close_transaction();

  std::vector<Symbol> find_symbol(const char *name) const;
  std::vector<Symbol> find_definition(const char *name) const;
  std::vector<Symbol> find_caller(const char *name) const;
  std::vector<Symbol> find_call(const char *name) const;
  std::vector<std::string> find_file(const char *name) const;
  std::vector<Symbol> find_includer(const char *name) const;

  // Give callers the convenience of passing a string instead.
  std::vector<Symbol> find_symbol(const std::string &name) const {
    return find_symbol(name.c_str());
  }
  std::vector<Symbol> find_definition(const std::string &name) const {
    return find_definition(name.c_str());
  }
  std::vector<Symbol> find_caller(const std::string &name) const {
    return find_caller(name.c_str());
  }
  std::vector<Symbol> find_call(const std::string &name) const {
    return find_call(name.c_str());
  }
  std::vector<std::string> find_file(const std::string &name) const {
    return find_file(name.c_str());
  }
  std::vector<Symbol> find_includer(const std::string &name) const {
    return find_includer(name.c_str());
  }

  void replay(const PendingActions &pending) {
    for (const std::string &s : pending.to_purge)
      (void)purge(s);
    for (const Symbol &s : pending.symbols)
      consume(s);
  }

 private:
  sqlite3 *m_db = nullptr;
  sqlite3_stmt *m_insert = nullptr;
  sqlite3_stmt *m_delete = nullptr;

};
