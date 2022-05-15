#pragma once

#include <clink/db.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CLINK_API
#define CLINK_API __attribute__((visibility("default")))
#endif

/** parse the given C/C++ file, inserting results into the given database
 *
 * This function is provided as an alternative to `clink_parse_c` for when the
 * action being done with every result is simply to insert it into a Clink
 * database. This function avoids the overhead of constructing and managing
 * iterator state.
 *
 * \param db Database to insert into
 * \param filename Path to source file to parse
 * \param argc Number of elements in argv
 * \param argv Arguments to pass to Clang
 * \returns 0 on success or an errno on failure
 */
CLINK_API int clink_parse_c_into(clink_db_t *db, const char *filename,
                                 size_t argc, const char **argv);

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
CLINK_API int clink_compiler_includes(const char *compiler, char ***includes,
                                      size_t *includes_len);

#ifdef __cplusplus
}
#endif
