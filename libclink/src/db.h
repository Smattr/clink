#pragma once

#include "re.h"
#include <pthread.h>
#include <sqlite3.h>
#include <stdbool.h>

struct clink_db {

  /// path to the database file
  char *path;

  /// handle to backing SQLite database
  sqlite3 *db;

  /// pre-compiled regexes
  re_t *regexes;

  /// mutual exclusion mechanism to accelerate bulk operations
  pthread_mutex_t bulk_operation;
  bool bulk_operation_inited : 1;
};
