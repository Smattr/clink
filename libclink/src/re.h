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

/** cleanup pre-compiled regexes
 *
 * The input to this function is assumed to be a non-null double pointer to a
 * list of regexes, \p re_t**. It is typed as \p void* for interaction with the
 * SQLite API.
 *
 * \param re List of regexes to deallocate
 */
INTERNAL void re_free(void *re);
