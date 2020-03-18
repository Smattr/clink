#include <assert.h>
#include <clink/db.h>
#include <errno.h>
#include "error.h"
#include "sql.h"
#include <sqlite3.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>

static const char SYMBOLS_SCHEMA[] = "create table if not exists symbols (name "
  "text not null, path text not null, category integer not null, line integer "
  "not null, col integer not null, parent text, "
  "unique(name, path, category, line, col));";

static const char CONTENT_SCHEMA[] = "create table if not exists content "
  "(path text not null, line integer not null, body text not null, "
  "unique(path, line));";

static const char *PRAGMAS[] = {
  "pragma synchronous=OFF;",
  "pragma journal_mode=MEMORY;",
  "pragma temp_store=MEMORY;",
};

static int init(sqlite3 *db) {

  assert(db != NULL);

  int rc = 0;

  if ((rc = sql_exec(db, SYMBOLS_SCHEMA)) != SQLITE_OK)
    return sqlite_error(rc);

  if ((rc = sql_exec(db, CONTENT_SCHEMA)) != SQLITE_OK)
    return sqlite_error(rc);

  for (size_t i = 0; i < sizeof(PRAGMAS) / sizeof(PRAGMAS[0]); i++)
    if ((rc = sql_exec(db, PRAGMAS[i])) != SQLITE_OK)
      return sqlite_error(rc);

  return rc;
}

int clink_db_open(struct clink_db *db, const char *path) {

  assert(db != NULL);
  assert(path != NULL);

  memset(db, 0, sizeof(*db));

  // check if the file exists, so we know whether to create the database
  // structure
  bool exists = !(access(path, R_OK|W_OK) == -1 && errno == ENOENT);

  // open the database, which may or may not exist
  int rc = sqlite3_open(path, &db->handle);
  if (rc != SQLITE_OK)
    return sqlite_error(rc);

  // if it did not exist, create its structure now
  if (!exists)
    if ((rc = init(db->handle)) != 0)
      return rc;

  return rc;
}
