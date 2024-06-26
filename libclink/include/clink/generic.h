#pragma once

#include <clink/db.h>

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

/// comment format of a source code language
typedef struct {
  const char
      *start;      ///< sequence of characters that begins this type of comment
  const char *end; ///< Sequence of characters that ends this type of comment.
                   ///< `NULL` means “end of line”.
  unsigned escapes : 1; ///< does this type of comment support `\\` escapes?
} clink_comment_t;

/// description of how to parse a source language
typedef struct {
  const char *
      *keywords; ///< Words that should never be considered references. A \p
                 ///< NULL entry is expected to terminate this array.
  const char *
      *defn_leaders; ///< Words that indicate the next symbol is a definition. A
                     ///< `NULL` entry is expected to terminate this array.
  clink_comment_t *comments; ///< Comment formats. A `{ 0, 0 }` entry is
                             ///< expected to terminate this array.
} clink_lang_t;

/** parse the given source in a language-agnostic way
 *
 * This function is intended to be used for parsing a language Clink does not
 * have a detailed understanding of. It is only capable of inferring definitions
 * (`CLINK_DEFINITION`) and references (`CLINK_REFERENCE`), and even these are
 * just educated guesses.
 *
 * The comment start and end markers defined in `lang->comments` cannot include
 * newline characters ('\n' and '\r').
 *
 * The `filename` parameter must be an absolute path.
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
