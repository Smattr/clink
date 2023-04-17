#pragma once

#include <clink/db.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CLINK_API
#define CLINK_API __attribute__((visibility("default")))
#endif

/** parse the given TableGen file, inserting results into the given database
 *
 * libclink has a limited understanding of TableGen. Results will be
 * approximate.
 *
 * \param db Database to insert into
 * \param filename Path to source file to parse
 * \return 0 on success or an errno on failure
 */
CLINK_API int clink_parse_tablegen(clink_db_t *db, const char *filename);

#ifdef __cplusplus
}
#endif
