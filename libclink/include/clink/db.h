#pragma once

#include <clink/symbol.h>
#include <sqlite3.h>

#ifdef __cplusplus
extern "C" {
#endif

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

/** find occurrences of a symbol in a Clink symbol database
 *
 * @param db Database to search
 * @param name Symbol to look for
 * @param callback This is invoked for each instance of the symbol found. Return
 *   a non-zero value to terminate the search.
 * @return 0 on success, a Clink error code or the returned value from the
 *   caller’s callback on failure
 */
int clink_db_find_symbol(struct clink_db *db, const char *name,
  int (*callback)(const struct clink_result *result));

/** close a Clink symbol database
 *
 * @param db Database structure to close and deallocate members from
 */
void clink_db_close(struct clink_db *db);

#ifdef __cplusplus
}
#endif
