#pragma once

#include <clink/iter.h>

#ifdef __cplusplus
extern "C" {
#endif

/** open Vim at the given position in the given file
 *
 * \param filename File to open with Vim
 * \param lineno Line number to position cursor at within the file
 * \param colno Column number to position cursor at within the file
 * \returns Vim’s exit status
 */
int clink_vim_open(const char *filename, unsigned long lineno,
  unsigned long colno);

/** create a string iterator for the Vim-highlighted lines of the given file
 *
 * \param it [out] Created iterator on success
 * \param filename Source file to read
 * \returns 0 on success, an errno on failure
 */
int clink_vim_highlight(clink_iter_t **it, const char *filename);

#ifdef __cplusplus
}
#endif
