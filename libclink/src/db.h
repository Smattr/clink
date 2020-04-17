#pragma once

#include <sqlite3.h>

struct clink_db {

  /// handle to backing SQLite database
  sqlite3 *db;
};
