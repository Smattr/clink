#pragma once

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

#if 0
/** yield syntax highlighted lines from a file, as if displayed by Vim
 *
 * \param filename Source file to read
 * \param callback Caller function to receive syntax highlighted lines
 * \returns 0 on success, an errno on failure
 */
int vim_highlight(const std::string &filename,
  std::function<int(const std::string&)> const &callback);
#endif

#ifdef __cplusplus
}
#endif
