#include <cassert>
#include "Database.h"
#include "errno.h"
#include <sqlite3.h>
#include <unistd.h>

static const char SYMBOLS_SCHEMA[] = "create table if not exists symbols (name "
    "text not null, type integer not null, line integer not null, col integer "
    "not null);";

static int init(sqlite3 *db) {
    assert(db != nullptr);

    if (sqlite3_exec(db, SYMBOLS_SCHEMA, nullptr, nullptr, nullptr)
            != SQLITE_OK)
        return -1;

    return 0;
}

bool Database::open(const char *path) {

    /* Fail if we already have a database open. */
    if (m_db)
        return false;

    /* Check if the file exists, so we know whether to create the database
     * structure.
     */
    bool exists = !(access(path, R_OK|W_OK) == -1 && errno == ENOENT);

    if (sqlite3_open(path, &m_db) != SQLITE_OK)
        return false;

    if (!exists) {
        if (init(m_db) != 0)
            return false;
    }

    return true;
}

void Database::close() {
    if (m_db) {
        sqlite3_close(m_db);
        m_db = nullptr;
    }
}

Database::~Database() {
    if (m_db)
        close();
}
