#pragma once

#include <sqlite3.h>

class Database {

public:
    ~Database();
    bool open(const char *path);
    void close();

private:
    sqlite3 *m_db = nullptr;

};
