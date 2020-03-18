#pragma once

#include <sqlite3.h>

struct clink_db {
  sqlite3 *handle;
};

/** open a Clink symbol database
 *
 * @param[out] db Database structure to populate
 * @param path File system path to the database file
 * @return 0 on success, a Clink error code on failure
 */
int clink_db_open(struct clink_db *db, const char *path);
