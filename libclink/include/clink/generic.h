#pragma once

#include <clink/db.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CLINK_API
#define CLINK_API __attribute__((visibility("default")))
#endif

/// description of how to parse a source language
typedef struct {
  const char **keywords;  ///< words that should never be considered references
  size_t keywords_length; ///< number of entries in \p keywords
  const char *
      *defn_leaders; ///< words that indicate the next symbol is a definition
  size_t defn_leaders_length; ///< number of entries in \p defn_leaders
} clink_lang_t;

/** parse the given source in a language-agnostic way
 *
 * This function is intended to be used for parsing a language Clink does not
 * have a detailed understanding of. It is only capable of inferring definitions
 * (`CLINK_DEFINITION`) and references (`CLINK_REFERENCE`), and even these are
 * just educated guesses.
 *
 * \param db Database to insert into
 * \param filename Path to source file to parse
 * \param lang The source language for interpreting this file
 * \return 0 on success or an errno on failure
 */
CLINK_API int clink_parse_generic(clink_db_t *db, const char *filename,
                                  const clink_lang_t *lang);

#ifdef __cplusplus
}
#endif
