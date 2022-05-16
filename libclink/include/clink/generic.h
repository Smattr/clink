#pragma once

#include <clink/db.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CLINK_API
#define CLINK_API __attribute__((visibility("default")))
#endif

/** parse the given source in a language-agnostic way
 *
 * This function is intended to be used for parsing a language Clink does not
 * have a detailed understanding of. It is only capable of inferring references
 * (`CLINK_REFERENCE`), and even these are just educated guesses.
 *
 * \param db Database to insert into
 * \param filename Path to source file to parse
 * \param keywords A list of words that should never be considered references
 * \param keywords_length The number of entries in `keywords`
 * \returns 0 on success or an errno on failure
 */
CLINK_API int clink_parse_generic(clink_db_t *db, const char *filename,
                                  const char **keywords,
                                  size_t keywords_length);

#ifdef __cplusplus
}
#endif
