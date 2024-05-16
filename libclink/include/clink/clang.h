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

/** parse the given C/C++ file with Clang
 *
 * The `filename` parameter must be an absolute path.
 *
 * \param db Database to insert into
 * \param filename Path to source file to parse
 * \param argc Number of Clang command line arguments
 * \param argv Clang commang line arguments
 * \return 0 on success or an errno on failure
 */
CLINK_API int clink_parse_with_clang(clink_db_t *db, const char *filename,
                                     size_t argc, const char **argv);

#ifdef __cplusplus
}
#endif
