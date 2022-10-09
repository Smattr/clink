#pragma once

#include <clink/db.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CLINK_API
#define CLINK_API __attribute__((visibility("default")))
#endif

/** is \p cscope available?
 *
 * \return True if libclink can find \p cscope
 */
CLINK_API bool clink_have_cscope(void);

/** parse the given file with Cscope, inserting results into the given database
 *
 * This function encapsulates executing \p cscope, parsing the database it
 * writes back into memory, and then writing this into a Clink database.
 *
 * \param db Database to insert into
 * \param filename Path to source file to parse
 * \return 0 on success or an errno on failure
 */
CLINK_API int clink_parse_with_cscope(clink_db_t *db, const char *filename);

#ifdef __cplusplus
}
#endif
