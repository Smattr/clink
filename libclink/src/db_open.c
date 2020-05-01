#include <assert.h>
#include <clink/db.h>
#include "db.h"
#include <errno.h>
#include "sql.h"
#include <sqlite3.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

static const char SYMBOLS_SCHEMA[] = "create table if not exists symbols (name "
  "text not null, path text not null, category integer not null, line integer "
  "not null, col integer not null, parent text, "
  "unique(name, path, category, line, col));";

static const char CONTENT_SCHEMA[] = "create table if not exists content "
  "(path text not null, line integer not null, body text not null, "
  "unique(path, line));";

static const char RECORDS_SCHEMA[] = "create table if not exists records "
  "(path text not null unique, hash integer not null, timestamp integer not "
  "null);";

static const char *PRAGMAS[] = {
  "pragma synchronous=OFF;",
  "pragma journal_mode=MEMORY;",
  "pragma temp_store=MEMORY;",
};

static int init(sqlite3 *db) {

  assert(db != NULL);

  int rc = 0;

  if ((rc = sql_exec(db, SYMBOLS_SCHEMA)))
    return rc;

  if ((rc = sql_exec(db, CONTENT_SCHEMA)))
    return rc;

  if ((rc = sql_exec(db, RECORDS_SCHEMA)))
    return rc;

  for (size_t i = 0; i < sizeof(PRAGMAS) / sizeof(PRAGMAS[0]); ++i) {
    if ((rc = sql_exec(db, PRAGMAS[i])))
      return rc;
  }

  return rc;
}


int clink_db_open(clink_db_t **db, const char *path) {

  if (db == NULL)
    return EINVAL;

  if (path == NULL)
    return EINVAL;

  clink_db_t *d = calloc(1, sizeof(*d));
  if (d == NULL)
    return ENOMEM;

  int rc = 0;

  // check if the database file already exists, so we know whether to create the
  // database structure
  bool exists = !(access(path, R_OK|W_OK) == -1 && errno == ENOENT);

  int err = sqlite3_open(path, &d->db);
  if (err != SQLITE_OK) {
    rc = sql_err_to_errno(err);
    goto done;
  }

  if (!exists)
    rc = init(d->db);

done:
  if (rc) {
    if (d != NULL && d->db != NULL)
      (void)sqlite3_close(d->db);
    if (!exists)
      (void)unlink(path);
    free(d);
  } else {
    *db = d;
  }

  return rc;
}
