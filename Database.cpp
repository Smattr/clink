#include <cassert>
#include "Database.h"
#include "errno.h"
#include <sqlite3.h>
#include <unistd.h>

static const char SYMBOLS_SCHEMA[] = "create table if not exists symbols (name "
    "text not null, path text not null, category integer not null, line "
    "integer not null, col integer not null, unique(name, path, category, "
    "line, col));";

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
        if (m_insert)
            sqlite3_finalize(m_insert);
        sqlite3_close(m_db);
        m_db = nullptr;
    }
}

bool Database::open_transaction() {
    assert(m_db != nullptr);
    return sqlite3_exec(m_db, "begin transaction;", nullptr, nullptr, nullptr)
        == SQLITE_OK;
}

bool Database::close_transaction() {
    assert(m_db != nullptr);
    return sqlite3_exec(m_db, "commit transaction;", nullptr, nullptr, nullptr)
        == SQLITE_OK;
}

void Database::consume(const Symbol &s) {
    assert(m_db != nullptr);

    if (m_insert == nullptr) {
        if (sqlite3_prepare_v2(m_db, "insert into symbols (name, path, "
                "category, line, col) values (@name, @path, @category, "
                "@line, @col);", -1, &m_insert, nullptr) != SQLITE_OK)
            return;
    } else {
        if (sqlite3_reset(m_insert) != SQLITE_OK)
            return;
    }

    int index = sqlite3_bind_parameter_index(m_insert, "@name");
    assert(index != 0);
    if (sqlite3_bind_text(m_insert, index, s.name, -1, SQLITE_STATIC)
            != SQLITE_OK)
        return;

    index = sqlite3_bind_parameter_index(m_insert, "@path");
    assert(index != 0);
    if (sqlite3_bind_text(m_insert, index, s.path, -1, SQLITE_STATIC)
            != SQLITE_OK)
        return;

    index = sqlite3_bind_parameter_index(m_insert, "@category");
    assert(index != 0);
    if (sqlite3_bind_int(m_insert, index, s.category) != SQLITE_OK)
        return;

    index = sqlite3_bind_parameter_index(m_insert, "@line");
    assert(index != 0);
    if (sqlite3_bind_int(m_insert, index, s.line) != SQLITE_OK)
        return;

    index = sqlite3_bind_parameter_index(m_insert, "@col");
    assert(index != 0);
    if (sqlite3_bind_int(m_insert, index, s.col) != SQLITE_OK)
        return;

    if (sqlite3_step(m_insert) != SQLITE_DONE)
        return;
}

bool Database::purge(const char *path) {
    assert(path != nullptr);

    if (m_delete == nullptr) {
        if (sqlite3_prepare_v2(m_db, "delete from symbols where path = @path;",
                -1, &m_delete, nullptr) != SQLITE_OK)
            return false;
    } else {
        if (sqlite3_reset(m_delete) != SQLITE_OK)
            return false;
    }

    int index = sqlite3_bind_parameter_index(m_delete, "@path");
    assert(index != 0);
    if (sqlite3_bind_text(m_delete, index, path, -1, SQLITE_STATIC)
            != SQLITE_OK)
        return false;

    if (sqlite3_step(m_delete) != SQLITE_DONE)
        return false;

    return true;
}

Database::~Database() {
    if (m_db)
        close();
}
