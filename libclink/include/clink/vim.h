#pragma once

#include <clink/db.h>
#include <clink/iter.h>

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

/** create a string iterator for the Vim-highlighted lines of the given file
 *
 * \param it [out] Created iterator on success
 * \param filename Source file to read
 * \returns 0 on success, an errno on failure
 */
CLINK_API int clink_vim_highlight(clink_iter_t **it, const char *filename);

/** Vim-highlight the given file, inserting results into the given database
 *
 * This function is provided as an alternative to `clink_vim_highlight` for when
 * the action being done with every result is simply to insert it into a Clink
 * database. This function avoids the overhead of constructing and managing
 * iterator state.
 *
 * \param db Database to insert into
 * \param filename Source file to read
 * \returns 0 on success or an errno on failure
 */
CLINK_API int clink_vim_highlight_into(clink_db_t *db, const char *filename);

#ifdef __cplusplus
}
#endif
