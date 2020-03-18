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

/** remove records related to a given source file from a Clink symbol database
 *
 * @param db Database structure to operate on
 * @param path Source file path for which to remove records
 * @return 0 on success, a Clink error code on failure
 */
int clink_db_remove(struct clink_db *db, const char *path);

/** close a Clink symbol database
 *
 * @param db Database structure to close and deallocate members from
 */
void clink_db_close(struct clink_db *db);
