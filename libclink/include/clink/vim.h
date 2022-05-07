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
 * \returns Vim’s exit status
 */
CLINK_API int clink_vim_open(const char *filename, unsigned long lineno,
                             unsigned long colno);

/** Vim-highlight the given file, returning lines through the callback function
 *
 * The callback function receives lines that have been syntax-highlighted using
 * ANSI terminal escape sequences. Iteration through the file will be terminated
 * when the end of file is reached or the caller’s callback returns non-zero.
 *
 * \param filename Source file to read
 * \param callback Handler for highlighted lines
 * \param state State to pass as first parameter to the callback
 * \returns 0 on success, an errno on failure, or the last non-zero return from
 *   the caller’s callback if there was one
 */
CLINK_API int clink_vim_read(const char *filename,
                             int (*callback)(void *state, const char *line),
                             void *state);

/** Vim-highlight the given file, inserting results into the given database
 *
 * This function is provided as an alternative to `clink_vim_read` for when the
 * action being done with every result is simply to insert it into a Clink
 * database.
 *
 * \param db Database to insert into
 * \param filename Source file to read
 * \returns 0 on success or an errno on failure
 */
CLINK_API int clink_vim_read_into(clink_db_t *db, const char *filename);

#ifdef __cplusplus
}
#endif
