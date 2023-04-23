#pragma once

#include <clink/db.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CLINK_API
#define CLINK_API __attribute__((visibility("default")))
#endif

/** open Vim at the given position in the given file
 *
 * \param filename File to open with Vim
 * \param lineno Line number to position cursor at within the file
 * \param colno Column number to position cursor at within the file
 * \param cscopeprg Optional value to set Vim’s \p cscopeprg variable to
 * \param db Optional database to connect Vim to
 * \return Vim’s exit status
 */
CLINK_API int clink_vim_open(const char *filename, unsigned long lineno,
                             unsigned long colno, const char *cscopeprg,
                             const clink_db_t *db);

/** Vim-highlight the given file, returning lines through the callback function
 *
 * The callback function receives lines that have been syntax-highlighted using
 * ANSI terminal escape sequences. Iteration through the file will be terminated
 * when the end of file is reached or the caller’s callback returns non-zero.
 *
 * \param filename Source file to read
 * \param callback Handler for highlighted lines
 * \param state State to pass as first parameter to the callback
 * \return 0 on success, an errno on failure, or the last non-zero return from
 *   the caller’s callback if there was one
 */
CLINK_API int clink_vim_read(const char *filename,
                             int (*callback)(void *state, char *line),
                             void *state);

/** Vim-highlight the given file, inserting results into the given database
 *
 * This function is provided as an alternative to \p clink_vim_read for when the
 * action being done with every result is simply to insert it into a Clink
 * database.
 *
 * Only lines for which the database contains a symbol reference will be stored.
 * That is, it is assumed the caller has previously \p clink_db_add_symbol any
 * symbols whose line content they wish to be added.
 *
 * The \p filename parameter must be an absolute path.
 *
 * \param db Database to insert into
 * \param filename Source file to read
 * \return 0 on success or an errno on failure
 */
CLINK_API int clink_vim_read_into(clink_db_t *db, const char *filename);

#ifdef __cplusplus
}
#endif
