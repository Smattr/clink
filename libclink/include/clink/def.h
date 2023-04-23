#pragma once

#include <clink/db.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CLINK_API
#define CLINK_API __attribute__((visibility("default")))
#endif

/** parse an MSVC Module-Definition file
 *
 * The \p filename parameter must be an absolute path.
 *
 * \param db Database to insert into
 * \param filename Path to .def file to parse
 * \return 0 on success or an errno on failure
 */
CLINK_API int clink_parse_def(clink_db_t *db, const char *filename);

#ifdef __cplusplus
}
#endif
