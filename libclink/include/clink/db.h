#pragma once

#include <clink/iter.h>
#include <clink/symbol.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CLINK_API
#define CLINK_API __attribute__((visibility("default")))
#endif

typedef struct clink_db clink_db_t;

/** open a Clink symbol database, creating it if it does not exist
 *
 * \param db [out] Handle to the opened database on success
 * \param path Path to the database file to open
 * \return 0 on success or an errno on failure
 */
CLINK_API int clink_db_open(clink_db_t **db, const char *path);

/** start a transaction on the underlying database store
 *
 * Addition operations to a database will run faster inside a transaction. So if
 * you are about to repeatedly call addition functions, it is a good idea to
 * call this function first.
 *
 * A transaction is closed by calling `clink_db_commit_transaction`. There is no
 * way to rollback/abort a transaction. Closing a database while a transaction
 * is still open results in undefined behaviour. Transactions do not nest. That
 * is, you calling this function twice without an intervening
 * `clink_db_commit_transaction` will result in an error.
 *
 * \param db Database to operate on
 * \return 0 on success or an errno on failure
 */
CLINK_API int clink_db_begin_transaction(clink_db_t *db);

/** close a transaction and commit its pending actions
 *
 * Calling this without a transaction being in progress (i.e. without previously
 * calling `clink_db_begin_transaction`) is an error.
 *
 * \param db Database to operate on
 * \return 0 on success or an errno on failure
 */
CLINK_API int clink_db_commit_transaction(clink_db_t *db);

/// identifier/handle for a record
typedef int64_t clink_record_id_t;

/** add a file record to the database
 *
 * Both the hash and the timestamp are uninterpreted internally, so the caller
 * can choose any representation they desire.
 *
 * The `filename` parameter must be an absolute path.
 *
 * \param db Database to operate on
 * \param path Path of the subject to store information about
 * \param hash Some hash digest of the subject
 * \param timestamp Last modification time of the subject
 * \param id [out] Identifier of the inserted record on success
 * \return 0 on success or an errno on failure
 */
CLINK_API int clink_db_add_record(clink_db_t *db, const char *path,
                                  uint64_t hash, uint64_t timestamp,
                                  clink_record_id_t *id);

/** add a symbol to the database
 *
 * `symbol->path` must be an absolute path.
 *
 * \param db Database to operate on
 * \param symbol Symbol to add
 * \return 0 on success or a SQLite error code on failure
 */
CLINK_API int clink_db_add_symbol(clink_db_t *db, const clink_symbol_t *symbol);

/** add a line of source content to the database
 *
 * The `path` parameter must be an absolute path.
 *
 * \param db Database to operate on
 * \param path Path of the file this line came from
 * \param lineno Line number within the file this came from
 * \param line Content of the line itself
 * \return 0 on success or an errno on failure
 */
CLINK_API int clink_db_add_line(clink_db_t *db, const char *path,
                                unsigned long lineno, const char *line);

/** remove all symbols and content related to a given file
 *
 * The `path` parameter must be an absolute path.
 *
 * \param db Clink database to operate on
 * \param path Path of the file to remove information for
 */
CLINK_API void clink_db_remove(clink_db_t *db, const char *path);

/** find function calls within a given function in the database
 *
 * Symbols paths in the returned iterator are always absolute.
 *
 * \param db Database to search
 * \param regex Regular expression of a containing function to lookup
 * \param it [out] Created symbol iterator on success
 * \return 0 on success or an errno on failure
 */
CLINK_API int clink_db_find_call(clink_db_t *db, const char *regex,
                                 clink_iter_t **it);

/** find calls to a given function in the database
 *
 * Symbols paths in the returned iterator are always absolute.
 *
 * \param db Database to search
 * \param regex Regular expression of a function whose calls to lookup
 * \param it [out] Created symbol iterator on success
 * \return 0 on success or an errno on failure
 */
CLINK_API int clink_db_find_caller(clink_db_t *db, const char *regex,
                                   clink_iter_t **it);

/** find a definition in the database
 *
 * Symbols paths in the returned iterator are always absolute.
 *
 * \param db Database to search
 * \param regex Regular expression of a symbol to search for
 * \param it [out] Created symbol iterator on success
 * \return 0 on success or an errno on failure
 */
CLINK_API int clink_db_find_definition(clink_db_t *db, const char *regex,
                                       clink_iter_t **it);

/** find #includes or a given file in the database
 *
 * Symbols paths in the returned iterator are always absolute.
 *
 * \param db Database to search
 * \param regex Regular expressions of filename of the function being #included
 * \param it [out] Created symbol iterator on success
 * \return 0 on success or an errno on failure
 */
CLINK_API int clink_db_find_includer(clink_db_t *db, const char *regex,
                                     clink_iter_t **it);

/** find a record in the given database
 *
 * The hash and timestamp parameters can be NULL if the caller does not need
 * this information.
 *
 * The `path` parameter must be an absolute path.
 *
 * \param db Database to search
 * \param path Path to subject to lookup
 * \param hash [out] Hash digest of the subject if a record was found
 * \param timestamp [out] Modification time of the subject if a record was found
 * \return 0 if a record was found, ENOENT if no record was found, or another
 *   errno on failure
 */
CLINK_API int clink_db_find_record(clink_db_t *db, const char *path,
                                   uint64_t *hash, uint64_t *timestamp);

/** find a symbol in the database
 *
 * Symbols paths in the returned iterator are always absolute.
 *
 * \param db Database to search
 * \param regex Regular expression of the symbol to lookup
 * \param it [out] Created symbol iterator on success
 * \return 0 on success or an errno on failure
 */
CLINK_API int clink_db_find_symbol(clink_db_t *db, const char *regex,
                                   clink_iter_t **it);

/** retrieve a highlighted line from the database
 *
 * The `path` parameter must be an absolute path.
 *
 * \param db Database to search
 * \param path Path to file whose content to retrieve
 * \param lineno Line number of the line to retrieve
 * \param content [out] Highlighted line content on success
 * \return 0 on success or an errno on failure.
 */
CLINK_API int clink_db_get_content(clink_db_t *db, const char *path,
                                   unsigned long lineno, char **content);

/** close a Clink symbol database
 *
 * \param db Database to close
 */
CLINK_API void clink_db_close(clink_db_t **db);

#ifdef __cplusplus
}
#endif
