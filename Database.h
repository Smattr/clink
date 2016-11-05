#pragma once

#include <sqlite3.h>
#include "Symbol.h"

class Database : public SymbolConsumer {

public:
    ~Database();
    bool open(const char *path);
    void close();
    void consume(const Symbol &s) override;

private:
    sqlite3 *m_db = nullptr;
    sqlite3_stmt *m_insert = nullptr;

};
