#pragma once

#include <clink/db.h>
#include <stdbool.h>
#include <stdio.h>

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

/** parse a Cscope namefile
 *
 * A Cscope namefile is a file containing space-/tab-/newline-separated entries
 * of paths. Paths containing space, tab, or newline characters must be enclosed
 * in double quotes. This function does not check whether any of the parsed
 * entries point to existing on-disk files/directories.
 *
 * The `accept` function will be called once for each parsed entry, with the
 * entry as the first parameter. Returning non-zero from your `accept` callback
 * will terminate parsing and return the same non-zero value. Passing `NULL` as
 * `accept` will skip notification of parsed entries. This can be useful if you
 * just want to validate the correctness of a namefile.
 *
 * The `error` function will be called for any syntax error in the input.
 * Returning non-zero from your `error` callback will override the error return
 * which otherwise will be `EIO`. Passing `NULL` as `error` will skip
 * notification of syntax errors. Parsing cannot tolerate multiple errors; the
 * first syntax error will terminate parsing regardless of what `error` returns.
 * Other errors unrelated to the content of the input file can also occur, in
 * which case the `error` callback will not be invoked.
 *
 * \param in Stream to read from
 * \param accept Callback for parsed names
 * \param error Callback for syntax errors in the namefile
 * \param context State passed to callbacks
 * \return 0 on success or an errno on failure
 */
CLINK_API int
clink_parse_namefile(FILE *in, int (*accept)(const char *name, void *context),
                     int (*error)(unsigned long lineno, unsigned long colno,
                                  const char *message, void *context),
                     void *context);

#ifdef __cplusplus
}
#endif
