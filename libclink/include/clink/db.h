#pragma once

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

/** close a Clink symbol database
 *
 * \param db Database to close
 */
void clink_db_close(clink_db_t **db);

#ifdef __cplusplus
}
#endif
