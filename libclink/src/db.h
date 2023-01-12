#pragma once

#include "re.h"
#include <sqlite3.h>

struct clink_db {

  /// path to the database file
  char *path;

  /// handle to backing SQLite database
  sqlite3 *db;

  /// pre-compiled regexes
  re_t *regexes;
};
