#pragma once

#include <sqlite3.h>

struct clink_db {

  /// path to the database file
  char *path;

  /// handle to backing SQLite database
  sqlite3 *db;
};
