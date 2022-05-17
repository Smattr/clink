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
 * \param db Database to insert into
 * \param filename Path to source file to parse
 * \param argc Number of elements in argv
 * \param argv Arguments to pass to Clang
 * \returns 0 on success or an errno on failure
 */
CLINK_API int clink_parse_c(clink_db_t *db, const char *filename, size_t argc,
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
CLINK_API int clink_compiler_includes(const char *compiler, char ***includes,
                                      size_t *includes_len);

/// opaque handle to a compilation database
typedef struct clink_comp_db clink_comp_db_t;

/** parse a Clang compilation database
 *
 * \param db [out] Handle to the database on success
 * \param path Path to the directory containing a compile_commands.json
 * \returns 0 on success or an errno on failure
 */
CLINK_API int clink_comp_db_open(clink_comp_db_t **db, const char *path);

/** lookup command line arguments to compile a given source
 *
 * \param db Compilation database to search
 * \param path Source being compiled
 * \param argc [out] Number of elements in returned `argv`
 * \param argv [out] Arguments to the compiler when building this source
 * \returns 0 on success or an errno on failure
 */
CLINK_API int clink_comp_db_find(const clink_comp_db_t *db, const char *path,
                                 size_t *argc, char ***argv);

/** release a compilation database and deallocate resources
 *
 * `*db` is `NULL` on return.
 *
 * \param db [inout] Database to deallocate
 */
CLINK_API void clink_comp_db_close(clink_comp_db_t **db);

#ifdef __cplusplus
}
#endif
