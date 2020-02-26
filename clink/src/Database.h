#pragma once

#include <clink/clink.h>
#include <sqlite3.h>
#include <string>
#include "Symbol.h"
#include <unordered_map>
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
  void consume(const clink::Symbol &s) final {
    symbols.push_back(s);
  }
  void consume(const std::string &path, unsigned lineno,
      const std::string &line) final {
    auto it = lines.find(path);
    if (it == lines.end()) {
      std::unordered_map<unsigned, std::string> ls;
      ls[lineno] = line;
      lines[path] = ls;
    } else {
      it->second[lineno] = line;
    }
  }
  bool purge(const std::string &path) final {
    to_purge.push_back(path);
    return true;
  }

  virtual ~PendingActions() {}

 private:
  std::vector<clink::Symbol> symbols;
  std::vector<std::string> to_purge;
  std::unordered_map<std::string, std::unordered_map<unsigned, std::string>> lines;

  friend class Database;
};

class Database : public SymbolConsumer {

 public:
  virtual ~Database();
  bool open(const char *path);
  void close();
  void consume(const clink::Symbol &s) final;
  void consume(const std::string &path, unsigned lineno,
    const std::string &line) final;
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
    for (const clink::Symbol &s : pending.symbols)
      consume(s);
    for (auto &kv : pending.lines)
      for (auto &ls : kv.second)
        consume(kv.first, ls.first, ls.second);
  }

 private:
  sqlite3 *m_db = nullptr;
  sqlite3_stmt *m_symbol_insert = nullptr;
  sqlite3_stmt *m_content_insert = nullptr;
  sqlite3_stmt *m_symbols_delete = nullptr;
  sqlite3_stmt *m_content_delete = nullptr;

};
