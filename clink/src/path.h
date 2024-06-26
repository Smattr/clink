// utilities for dealing with file system paths

#pragma once

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/** get current working directory
 *
 * \param wd [out] Path to current working directory on success
 * \return 0 on success or an errno on failure
 */
int cwd(char **wd);

/** get the directory name of a given path
 *
 * This is a more sane version of POSIX dirname().
 *
 * \param path Path to a file or directory
 * \param dir [out] The containing directory on success
 * \return 0 on success or an errno on failure
 */
int dirname(const char *path, char **dir);

/** get a friendly-for-display path relative to the current directory
 *
 * The input path is assumed to be absolute.
 *
 * \param cur_dir Path to the current working directory
 * \param path Path to a file or directory
 * \return Path relative to the current directory, or a copy of the original
 *   path if this is not possible
 */
const char *disppath(const char *cur_dir, const char *path);

/** is this a path to an assembly file?
 *
 * \param path Path to assess
 * \return True if this is an assembly file
 */
bool is_asm(const char *path);

/** is this a path to a C file?
 *
 * \param path Path to assess
 * \return True if this is a C file
 */
bool is_c(const char *path);

/** is this a path to a C++ file?
 *
 * \param path Path to assess
 * \return True if this is a C++ file
 */
bool is_cxx(const char *path);

/** is this a path to an MSVC Module-Definition file?
 *
 * \param path Path to assess
 * \return True if this is an MSVC Module-Definition file
 */
bool is_def(const char *path);

/** is this a directory?
 *
 * \param path Path to assess
 * \return True if this is a directory
 */
bool is_dir(const char *path);

/** is this a regular file?
 *
 * \param path Path to assess
 * \return True if this is a regular file
 */
bool is_file(const char *path);

/** is this a path to a Lex/Flex file?
 *
 * \param path Path to assess
 * \return True if this is a Lex/Flex file
 */
bool is_lex(const char *path);

/** is this a path to a Python file?
 *
 * \param path Path to assess
 * \return True if this is a Python file
 */
bool is_python(const char *path);

/** is this a relative (as opposed to an absolute) path?
 *
 * \param path Path to inspect
 * \return True if this is a relative path
 */
static inline bool is_relative(const char *path) {
  assert(path != NULL);
  return path[0] != '/';
}

/** is this the root directory of the file system?
 *
 * \param path Path to a file or directory
 * \return True if this is the file system root
 */
bool is_root(const char *path);

/** is this a source language we recognise?
 *
 * \param path Path to assess
 * \return True if this is a file of an enabled source language
 */
bool is_source(const char *path);

/** is this a path to a TableGen file?
 *
 * \param path Path to assess
 * \return True if this is a TableGen file
 */
bool is_tablegen(const char *path);

/** is this a path to a Yacc/Bison file?
 *
 * \param path Path to assess
 * \return True if this is a Yacc/Bison file
 */
bool is_yacc(const char *path);

/** concatenate two paths
 *
 * \param branch Start of the new path
 * \param stem End of the new path
 * \param path [out] Resulting concatenation on success
 * \return 0 on success or an errno on failure
 */
int join(const char *branch, const char *stem, char **path);
