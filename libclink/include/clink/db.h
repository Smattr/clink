#pragma once

#include <clink/iter.h>
#include <clink/symbol.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct clink_db clink_db_t;

/** open a Clink symbol database, creating it if it does not exist
 *
 * \param db [out] Handle to the opened database on success
 * \param path Path to the database file to open
 * \returns 0 on success or an errno on failure
 */
int clink_db_open(clink_db_t **db, const char *path);

/** add a symbol to the database
 *
 * \param db Database to operate on
 * \param symbol Symbol to add
 * \returns 0 on success or a SQLite error code on failure
 */
int clink_db_add_symbol(clink_db_t *db, const clink_symbol_t *symbol);

/** add a line of source content to the database
 *
 * \param db Database to operate on
 * \param path Path of the file this line came from
 * \param lineno Line number within the file this came from
 * \param line Content of the line itself
 * \returns 0 on success or a SQLite error code on failure
 */
int clink_db_add_line(clink_db_t *db, const char *path, unsigned long lineno,
  const char *line);

/** remove all symbols and content related to a given file
 *
 * \param db Clink database to operate on
 * \param path Path of the file to remove information for
 */
void clink_db_remove(clink_db_t *db, const char *path);

/** find function calls within a given function in the database
 *
 * \param db Database to search
 * \param name Symbol name of the containing function to lookup
 * \param it [out] Created symbol iterator on success
 * \returns 0 on success or an errno on failure
 */
int clink_db_find_call(clink_db_t *db, const char *name, clink_iter_t **it);

/** find calls to a given function in the database
 *
 * \param db Database to search
 * \param name Symbol name of the function being called to lookup
 * \param it [out] Created symbol iterator on success
 * \returns 0 on success or an errno on failure
 */
int clink_db_find_caller(clink_db_t *db, const char *name, clink_iter_t **it);

/** find a definition in the database
 *
 * \param db Database to search
 * \param name Symbol name of the definition to lookup
 * \param it [out] Created symbol iterator on success
 * \returns 0 on success or an errno on failure
 */
int clink_db_find_definition(clink_db_t *db, const char *name,
  clink_iter_t **it);

/** find a symbol in the database
 *
 * \param db Database to search
 * \param name Name of the symbol to lookup
 * \param it [out] Created symbol iterator on success
 * \returns 0 on success or an errno on failure
 */
int clink_db_find_symbol(clink_db_t *db, const char *name, clink_iter_t **it);

/** close a Clink symbol database
 *
 * \param db Database to close
 */
void clink_db_close(clink_db_t **db);

#ifdef __cplusplus
}
#endif
