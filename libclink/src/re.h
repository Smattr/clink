#pragma once

#include "../../common/compiler.h"
#include <regex.h>
#include <sqlite3.h>

/// translate a POSIX regex.h error to an errno
INTERNAL int re_err_to_errno(int err);

/// SQLite user function to be installed as `REGEXP`
INTERNAL void re_sqlite(sqlite3_context *context, int argc,
                        sqlite3_value **argv);

/// linked-list of pre-compiled regexes
typedef struct re {
  char *expression; ///< text of the originating regex
  regex_t compiled; ///< pre-pared regex
  struct re *next;  ///< next element in the linked-list
} re_t;

/** compile a new regex and add it to the given list
 *
 * This function is thread-safe. Multiple threads can call into it or \p re_find
 * concurrently with the same \p re pointer.
 *
 * \param re List to add to
 * \param regex Regex to compile
 * \return 0 on success or an errno on failure
 */
INTERNAL int re_add(re_t **re, const char *regex);

/** lookup a previously compiled regex
 *
 * This function is thread-safe. Multiple threads can call into it or \p re_add
 * concurrently with the same \p re pointer.
 *
 * The regex being looked up is assumed to exist. Looking up a regex that you do
 * not know exists in the list results in undefined behaviour.
 *
 * \param re List to search
 * \param regex Originating regex text
 * \return The matching previously compiled regex
 */
INTERNAL regex_t re_find(const re_t **re, const char *regex);

/** cleanup pre-compiled regexes
 *
 * The input to this function is assumed to be a non-null double pointer to a
 * list of regexes, \p re_t**. It is typed as \p void* for interaction with the
 * SQLite API.
 *
 * \param re List of regexes to deallocate
 */
INTERNAL void re_free(void *re);
