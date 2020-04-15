#pragma once

#include <clink/iter.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/** create an iterator for parsing the given C/C++ file
 *
 * \param it [out] Created iterator on success
 * \param filename Path to source file to parse
 * \param argc Number of elements in argv
 * \param argv Arguments to pass to Clang
 * \returns 0 on success or an errno on failure
 */
int clink_parse_c(clink_iter_t **it, const char *filename, size_t argc,
  const char **argv);

/** get the built-in list of #include paths the compiler knows
 *
 * If you pass NULL as the compiler path, this will default to the environment
 * variable $CXX or "c++" if the environment variable is unset.
 *
 * \param compiler Path or command name of the compiler
 * \param includes [out] List of #include paths on success
 * \param includes_len [out] Number of elements stored to includes
 * \returns 0 on success or an errno on failure
 */
int clink_compiler_includes(const char *compiler, char ***includes,
  size_t *includes_len);

#ifdef __cplusplus
}
#endif
