// utilities for dealing with file system paths

#pragma once

/** make a given (possibly relative) path absolute
 *
 * \param path Path to make absolute
 * \param result [out] Absolute path on success
 * \returns 0 on success or an errno on failure
 */
int abspath(const char *path, char **result);

/** get current working directory
 *
 * \param wd [out] Path to current working directory on success
 * \returns 0 on success or an errno on failure
 */
int cwd(char **wd);
