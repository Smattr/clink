#pragma once

#include "../../common/compiler.h"
#include <sqlite3.h>

/// translate a POSIX regex.h error to an errno
INTERNAL int re_err_to_errno(int err);

/// SQLite user function to be installed as `REGEXP`
INTERNAL void re_sqlite(sqlite3_context *context, int argc,
                        sqlite3_value **argv);
