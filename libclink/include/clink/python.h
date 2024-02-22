#pragma once

#include <clink/db.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CLINK_API
#define CLINK_API __attribute__((visibility("default")))
#endif

/** parse the given Python file, inserting results into the given database
 *
 * This parsing interface is extremely limited, capable only of heuristically
 * recognising definitions and references.
 *
 * The `filename` parameter must be an absolute path.
 *
 * \param db Database to insert into
 * \param filename Path to source file to parse
 * \return 0 on success or an errno on failure
 */
CLINK_API int clink_parse_python(clink_db_t *db, const char *filename);

#ifdef __cplusplus
}
#endif
