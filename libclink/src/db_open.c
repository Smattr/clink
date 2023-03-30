#include "db.h"
#include "debug.h"
#include "re.h"
#include "sql.h"
#include <assert.h>
#include <clink/db.h>
#include <errno.h>
#include <pthread.h>
#include <sqlite3.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static const char SYMBOLS_SCHEMA[] =
    "create table if not exists symbols (name "
    "text not null, path text not null, category integer not null, line "
    "integer "
    "not null, col integer not null, parent text, "
    "unique(name, path, category, line, col));";

static const char CONTENT_SCHEMA[] =
    "create table if not exists content "
    "(path text not null, line integer not null, body text not null, "
    "unique(path, line));";

static const char RECORDS_SCHEMA[] =
    "create table if not exists records "
    "(path text not null unique, hash integer not null, timestamp integer not "
    "null);";

static const char *PRAGMAS[] = {
    "pragma synchronous=OFF;",
    "pragma journal_mode=OFF;",
    "pragma temp_store=MEMORY;",
};

static int init(sqlite3 *db) {

  assert(db != NULL);

  int rc = 0;

  if (ERROR((rc = sql_exec(db, SYMBOLS_SCHEMA))))
    return rc;

  if (ERROR((rc = sql_exec(db, CONTENT_SCHEMA))))
    return rc;

  if (ERROR((rc = sql_exec(db, RECORDS_SCHEMA))))
    return rc;

  for (size_t i = 0; i < sizeof(PRAGMAS) / sizeof(PRAGMAS[0]); ++i) {
    if (ERROR((rc = sql_exec(db, PRAGMAS[i]))))
      return rc;
  }

  return rc;
}

int clink_db_open(clink_db_t **db, const char *path) {

  if (ERROR(db == NULL))
    return EINVAL;

  if (ERROR(path == NULL))
    return EINVAL;

  clink_db_t *d = calloc(1, sizeof(*d));
  if (ERROR(d == NULL))
    return ENOMEM;

  // check if the database file already exists, so we know whether to create the
  // database structure
  bool exists = !(access(path, R_OK | W_OK) == -1 && errno == ENOENT);

  int rc = 0;

  d->path = strdup(path);
  if (ERROR(d->path == NULL)) {
    rc = ENOMEM;
    goto done;
  }

  int err = sqlite3_open(path, &d->db);
  if (err != SQLITE_OK) {
    rc = sql_err_to_errno(err);
    goto done;
  }

  if (!exists) {
    if (ERROR(rc = init(d->db)))
      goto done;
  }

  // install a SQLite user function that implements regex
  {
    int eTextRep = SQLITE_UTF8;
#ifdef SQLITE_DETERMINISTIC
    eTextRep |= SQLITE_DETERMINISTIC;
#endif
#ifdef SQLITE_DIRECTONLY
    eTextRep |= SQLITE_DIRECTONLY;
#endif
    int r =
        sqlite3_create_function_v2(d->db, "regexp", 2, eTextRep, &d->regexes,
                                   re_sqlite, NULL, NULL, re_free);
    if (ERROR(r != SQLITE_OK)) {
      rc = sql_err_to_errno(r);
      goto done;
    }
  }

  if (ERROR((rc = pthread_mutex_init(&d->bulk_operation, NULL))))
    goto done;
  d->bulk_operation_inited = true;

done:
  if (rc) {
    clink_db_close(&d);
    if (!exists)
      (void)unlink(path);
  } else {
    *db = d;
  }

  return rc;
}
