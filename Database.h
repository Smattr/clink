#pragma once

#include <sqlite3.h>
#include <string>
#include "Symbol.h"
#include <vector>

class Database : public SymbolConsumer {

public:
    ~Database();
    bool open(const char *path);
    void close();
    void consume(const Symbol &s) override;
    bool purge(const char *path) override;
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

private:
    sqlite3 *m_db = nullptr;
    sqlite3_stmt *m_insert = nullptr;
    sqlite3_stmt *m_delete = nullptr;

};
