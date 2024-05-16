#pragma once

#include <clink/db.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CLINK_API
#ifdef __GNUC__
#define CLINK_API __attribute__((visibility("default")))
#elif defined(_MSC_VER)
#define CLINK_API __declspec(dllexport)
#else
#define CLINK_API /* nothing */
#endif
#endif

/** parse the given C file, inserting results into the given database
 *
 * This parsing interface is extremely limited, capable only of heuristically
 * recognising definitions and references. Callers likely want
 * `clink_parse_with_clang` instead.
 *
 * The `filename` parameter must be an absolute path.
 *
 * \param db Database to insert into
 * \param filename Path to source file to parse
 * \return 0 on success or an errno on failure
 */
CLINK_API int clink_parse_c(clink_db_t *db, const char *filename);

/** parse the given C++ file, inserting results into the given database
 *
 * The `filename` parameter must be an absolute path.
 *
 * \param db Database to insert into
 * \param filename Path to source file to parse
 * \return 0 on success or an errno on failure
 */
CLINK_API int clink_parse_cxx(clink_db_t *db, const char *filename);

/** parse the given source with a C preprocessor
 *
 * This implementation of the C preprocessor is extremely limited. It can only
 * recognise `#if`, `#elif`, `#else`, `#ifdef`, `#ifndef`, and `#undef`. It is
 * designed to process the directives that Libclang does not represent in its
 * AST. That is, this is intended to be used in combination with
 * `clink_parse_with_clang` to produce a more complete understanding of a source
 * file.
 *
 * The `filename` parameter must be an absolute path.
 *
 * \param db Database to insert into
 * \param filename Path to source file to parse
 * \return 0 on success or an errno on failure
 */
CLINK_API int clink_parse_cpp(clink_db_t *db, const char *filename);

/** get the built-in list of #include paths the compiler knows
 *
 * If you pass NULL as the compiler path, this will default to the environment
 * variable $CXX or "c++" if the environment variable is unset.
 *
 * \param compiler Path or command name of the compiler
 * \param includes [out] List of #include paths on success
 * \param includes_len [out] Number of elements stored to includes
 * \return 0 on success or an errno on failure
 */
CLINK_API int clink_compiler_includes(const char *compiler, char ***includes,
                                      size_t *includes_len);

#ifdef __cplusplus
}
#endif
