#pragma once

#include <sqlite3.h>
#include "Symbol.h"
#include <vector>

class Database : public SymbolConsumer {

public:
    ~Database();
    bool open(const char *path);
    void close();
    void consume(const Symbol &s) override;
    bool purge(const char *path);
    bool open_transaction();
    bool close_transaction();

    std::vector<Symbol> find_symbol(const char *name);
    std::vector<Symbol> find_definition(const char *name);
    std::vector<Symbol> find_caller(const char *name);

private:
    sqlite3 *m_db = nullptr;
    sqlite3_stmt *m_insert = nullptr;
    sqlite3_stmt *m_delete = nullptr;

};
