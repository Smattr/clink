#include <cassert>
#include "Database.h"
#include "errno.h"
#include <sqlite3.h>
#include <string.h>
#include "Symbol.h"
#include <unistd.h>
#include <vector>

using namespace std;

static const char SYMBOLS_SCHEMA[] = "create table if not exists symbols (name "
    "text not null, path text not null, category integer not null, line "
    "integer not null, col integer not null, parent text, context text, "
    "unique(name, path, category, line, col));";

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
                "category, line, col, parent, context) values (@name, @path, "
                "@category, @line, @col, @parent, @context);", -1, &m_insert,
                nullptr) != SQLITE_OK)
            return;
    } else {
        if (sqlite3_reset(m_insert) != SQLITE_OK)
            return;
    }

    int index = 1;
    assert(index == sqlite3_bind_parameter_index(m_insert, "@name"));
    if (sqlite3_bind_text(m_insert, index, s.name(), -1, SQLITE_STATIC)
            != SQLITE_OK)
        return;

    index = 2;
    assert(index == sqlite3_bind_parameter_index(m_insert, "@path"));
    if (sqlite3_bind_text(m_insert, index, s.path(), -1, SQLITE_STATIC)
            != SQLITE_OK)
        return;

    index = 3;
    assert(index == sqlite3_bind_parameter_index(m_insert, "@category"));
    if (sqlite3_bind_int(m_insert, index, s.category()) != SQLITE_OK)
        return;

    index = 4;
    assert(index == sqlite3_bind_parameter_index(m_insert, "@line"));
    if (sqlite3_bind_int(m_insert, index, s.line()) != SQLITE_OK)
        return;

    index = 5;
    assert(index == sqlite3_bind_parameter_index(m_insert, "@col"));
    if (sqlite3_bind_int(m_insert, index, s.col()) != SQLITE_OK)
        return;

    index = 6;
    assert(index == sqlite3_bind_parameter_index(m_insert, "@parent"));
    if (sqlite3_bind_text(m_insert, index, s.parent(), -1, SQLITE_STATIC)
            != SQLITE_OK)
        return;

    index = 7;
    assert(index == sqlite3_bind_parameter_index(m_insert, "@context"));
    if (sqlite3_bind_text(m_insert, index, s.context(), -1, SQLITE_STATIC)
            != SQLITE_OK)
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

vector<Symbol> Database::find_symbol(const char *name) {
    assert(m_db != nullptr);

    vector<Symbol> vs;

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, "select path, category, line, col, parent, "
            "context from symbols where name = @name;", -1, &stmt, nullptr)
            != SQLITE_OK)
        goto done;

    if (sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC) != SQLITE_OK)
        goto done;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *path = (char*)sqlite3_column_text(stmt, 0);
        symbol_category_t cat = symbol_category_t(sqlite3_column_int(stmt, 1));
        unsigned line = unsigned(sqlite3_column_int(stmt, 2));
        unsigned col = unsigned(sqlite3_column_int(stmt, 3));
        const char *parent = (char*)sqlite3_column_text(stmt, 4);
        const char *context = (char*)sqlite3_column_text(stmt, 5);

        Symbol s(name, path, cat, line, col, parent, context);
        vs.push_back(s);
    }

done:
    if (stmt != nullptr)
        sqlite3_finalize(stmt);
    return vs;
}

vector<Symbol> Database::find_definition(const char *name) {
    assert(m_db != nullptr);

    vector<Symbol> vs;

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, "select path, line, col, parent, context "
            "from symbols where name = @name and category = @category;", -1,
            &stmt, nullptr) != SQLITE_OK)
        goto done;

    if (sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC) != SQLITE_OK)
        goto done;

    if (sqlite3_bind_int(stmt, 2, ST_DEFINITION) != SQLITE_OK)
        goto done;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *path = (char*)sqlite3_column_text(stmt, 0);
        unsigned line = unsigned(sqlite3_column_int(stmt, 1));
        unsigned col = unsigned(sqlite3_column_int(stmt, 2));
        const char *parent = (char*)sqlite3_column_text(stmt, 3);
        const char *context = (char*)sqlite3_column_text(stmt, 4);

        Symbol s(name, path, ST_DEFINITION, line, col, parent, context);
        vs.push_back(s);
    }

done:
    if (stmt != nullptr)
        sqlite3_finalize(stmt);
    return vs;
}

vector<Symbol> Database::find_caller(const char *name) {
    assert(m_db != nullptr);

    vector<Symbol> vs;

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, "select path, line, col, parent, context "
            "from symbols where name = @name and category = @category;", -1,
            &stmt, nullptr) != SQLITE_OK)
        goto done;

    if (sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC) != SQLITE_OK)
        goto done;

    if (sqlite3_bind_int(stmt, 2, ST_FUNCTION_CALL) != SQLITE_OK)
        goto done;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *path = (char*)sqlite3_column_text(stmt, 0);
        unsigned line = unsigned(sqlite3_column_int(stmt, 1));
        unsigned col = unsigned(sqlite3_column_int(stmt, 2));
        const char *parent = (char*)sqlite3_column_text(stmt, 3);
        const char *context = (char*)sqlite3_column_text(stmt, 4);

        Symbol s(name, path, ST_FUNCTION_CALL, line, col, parent, context);
        vs.push_back(s);
    }

done:
    if (stmt != nullptr)
        sqlite3_finalize(stmt);
    return vs;
}

Database::~Database() {
    if (m_db)
        close();
}
