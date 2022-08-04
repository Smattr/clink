// utilities for dealing with file system paths

#pragma once

#include <stdbool.h>
#include <stdint.h>

/** get current working directory
 *
 * \param wd [out] Path to current working directory on success
 * \returns 0 on success or an errno on failure
 */
int cwd(char **wd);

/** get the directory name of a given path
 *
 * This is a more sane version of POSIX dirname().
 *
 * \param path Path to a file or directory
 * \param dir [out] The containing directory on success
 * \returns 0 on success or an errno on failure
 */
int dirname(const char *path, char **dir);

/** get a friendly-for-display path relative to the current directory
 *
 * \param path Path to a file or directory
 * \param display [out] Path relative to the current directory, or a copy of the
 *   original path if this is not possible
 * \returns 0 on success or an errno on failure
 */
int disppath(const char *path, char **display);

/** is this a path to an assembly file?
 *
 * \param path Path to assess
 * \returns True if this is an assembly file
 */
bool is_asm(const char *path);

/** is this a path to a C file?
 *
 * \param path Path to assess
 * \returns True if this is a C file
 */
bool is_c(const char *path);

/** is this a path to a C++ file?
 *
 * \param path Path to assess
 * \returns True if this is a C++ file
 */
bool is_cxx(const char *path);

/** is this a path to an MSVC Module-Definition file?
 *
 * \param path Path to assess
 * \returns True if this is an MSVC Module-Definition file
 */
bool is_def(const char *path);

/** is this a directory?
 *
 * \param path Path to assess
 * \returns True if this is a directory
 */
bool is_dir(const char *path);

/** is this a regular file?
 *
 * \param path Path to assess
 * \returns True if this is a regular file
 */
bool is_file(const char *path);

/** is this the root directory of the file system?
 *
 * \param path Path to a file or directory
 * \returns True if this is the file system root
 */
bool is_root(const char *path);

/** is this a source language we recognise?
 *
 * \param path Path to assess
 * \returns True if this is a file of an enabled source language
 */
bool is_source(const char *path);

/** concatenate two paths
 *
 * \param branch Start of the new path
 * \param stem End of the new path
 * \param path [out] Resulting concatenation on success
 * \returns 0 on success or an errno on failure
 */
int join(const char *branch, const char *stem, char **path);
