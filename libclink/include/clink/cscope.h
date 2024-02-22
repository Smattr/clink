#pragma once

#include <clink/db.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CLINK_API
#define CLINK_API __attribute__((visibility("default")))
#endif

/** is `cscope` available?
 *
 * \return True if libclink can find `cscope`
 */
CLINK_API bool clink_have_cscope(void);

/** parse the given file with Cscope, inserting results into the given database
 *
 * This function encapsulates executing `cscope`, parsing the database it writes
 * back into memory, and then writing this into a Clink database.
 *
 * If the caller knows the identifier of the record for the source path
 * `filename`, they can pass this as `id`. If not, they can pass -1 to indicate
 * the callee needs to look this up. In this case, the `filename` provided must
 * be an absolute path.
 *
 * \param db Database to insert into
 * \param filename Path to source file to parse
 * \param id Identifier of the record of `filename`
 * \return 0 on success or an errno on failure
 */
CLINK_API int clink_parse_with_cscope(clink_db_t *db, const char *filename,
                                      clink_record_id_t id);

#ifdef __cplusplus
}
#endif
