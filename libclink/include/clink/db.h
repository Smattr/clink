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

/** add a line of file content to a Clink symbol database
 *
 * @param db Database structure to operate on
 * @param path Path to the origin file
 * @param line Text content of the line
 * @param lineno Line number of the line
 * @return 0 on success, a Clink error code on failure
 */
int clink_db_add_line(struct clink_db *db, const char *path,
  const char *line, unsigned long lineno);

/** add symbol to Clink symbol database
 *
 * @param db Database to operate on
 * @param s Symbol to add
 * @return 0 on success, a Clink error code on failure
 */
int clink_db_add_symbol(struct clink_db *db, const struct clink_symbol *s);

/** find function calls within another function in a Clink symbol database
 *
 * @param db Database to search
 * @param name Function to look within
 * @param callback This is invoked for each call found. Return a non-zero value
 *   to terminate the search.
 * @return 0 on success, a Clink error code or the returned value from the
 *   caller’s callback on failure
 */
int clink_db_find_call(struct clink_db *db, const char *name,
  int (*callback)(const struct clink_result *result));

/** find function calls to a symbol in a Clink symbol database
 *
 * @param db Database to search
 * @param name Function symbol to look for
 * @param callback This is invoked for each call of the symbol found. Return a
 *   non-zero value to terminate the search.
 * @return 0 on success, a Clink error code or the returned value from the
 *   caller’s callback on failure
 */
int clink_db_find_caller(struct clink_db *db, const char *name,
  int (*callback)(const struct clink_result *result));

/** find definitions of a symbol in a Clink symbol database
 *
 * @param db Database to search
 * @param name Symbol to look for
 * @param callback This is invoked for each definition of the symbol found.
 *   Return a non-zero value to terminate the search.
 * @return 0 on success, a Clink error code or the returned value from the
 *   caller’s callback on failure
 */
int clink_db_find_definition(struct clink_db *db, const char *name,
  int (*callback)(const struct clink_result *result));

/** find files matching the given name in a Clink symbol database
 *
 * @param db Database to search
 * @param name Filename to look for
 * @param callback This is invoked for each instance of the file found. Return a
 *   non-zero value to terminate the search.
 * @return 0 on success, a Clink error code or the returned value from the
 *   caller’s callback on failure
 */
int clink_db_find_file(struct clink_db *db, const char *name,
  int (*callback)(const char *path));

/** find files including given file in a Clink symbol database
 *
 * @param db Database to search
 * @param name Filename to look for in #includes
 * @param callback This is invoked for each instance of a matching #include
 *   found. Return a non-zero value to terminate the search.
 * @return 0 on success, a Clink error code or the returned value from the
 *   caller’s callback on failure
 */
int clink_db_find_includer(struct clink_db *db, const char *name,
  int (*callback)(const struct clink_result *result));

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
