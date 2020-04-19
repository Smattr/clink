// utilities for dealing with file system paths

#pragma once

#include <stdbool.h>

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

/** get the directory name of a given path
 *
 * The input path is assumed to be absolute.
 *
 * \param path Path to a file or directory
 * \param dir [out] The containing directory on success
 * \returns 0 on success or an errno on failure
 */
int dirname(const char *path, char **dir);

/** is this a path to an assembly file?
 *
 * \param path Path to assess
 * \returns True if this is an assembly file
 */
bool is_asm(const char *path);

/** is this a path to a C/C++ file?
 *
 * \param path Path to assess
 * \returns True if this is an C/C++ file
 */
bool is_c(const char *path);

/** is this the root directory of the file system?
 *
 * \param path Path to a file or directory
 * \returns True if this is the file system root
 */
bool is_root(const char *path);

/** concatenate two paths
 *
 * \param branch Start of the new path
 * \param stem End of the new path
 * \param path [out] Resulting concatenation on success
 * \returns 0 on success or an errno on failure
 */
int join(const char *branch, const char *stem, char **path);
