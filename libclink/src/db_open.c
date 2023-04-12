#include "db.h"
#include "debug.h"
#include "re.h"
#include "schema.h"
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

static int exec_all(sqlite3 *db, size_t queries_length, const char **queries) {

  assert(db != NULL);
  assert(queries_length == 0 || queries != NULL);

  int rc = 0;

  for (size_t i = 0; i < queries_length; ++i) {
    assert(queries[i] != NULL);
    if (ERROR((rc = sql_exec(db, queries[i]))))
      return rc;
  }

  return rc;
}

static int init(sqlite3 *db) {
  assert(db != NULL);
  return exec_all(db, SCHEMA_LENGTH, SCHEMA);
}

static int configure(sqlite3 *db) {

  assert(db != NULL);

  static const char *PRAGMAS[] = {
      "pragma synchronous=OFF;",
      "pragma journal_mode=OFF;",
      "pragma temp_store=MEMORY;",
      "pragma foreign_keys=ON;",
  };

  return exec_all(db, sizeof(PRAGMAS) / sizeof(PRAGMAS[0]), PRAGMAS);
}

static int check_schema_version(sqlite3 *db) {

  assert(db != NULL);

  static const char QUERY[] =
      "select value from metadata where key = \"schema_version\";";

  int rc = 0;

  sqlite3_stmt *stmt = NULL;
  if (ERROR((rc = sql_prepare(db, QUERY, &stmt))))
    goto done;

  {
    int r = sqlite3_step(stmt);
    if (ERROR(r != SQLITE_ROW)) {
      if (r == SQLITE_DONE) {
        rc = ENOENT;
      } else {
        rc = sql_err_to_errno(r);
      }
      goto done;
    }
  }

  const char *ours = schema_version();
  const char *theirs = (char *)sqlite3_column_text(stmt, 0);

  assert(ours != NULL);
  assert(theirs != NULL);

  // reject any version that does not match exactly
  if (strcmp(ours, theirs) != 0) {
    rc = EPROTO;
    goto done;
  }

done:
  if (stmt != NULL)
    sqlite3_finalize(stmt);

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

  if (exists) {
    if (ERROR((rc = check_schema_version(d->db))))
      goto done;
  } else {
    if (ERROR((rc = init(d->db))))
      goto done;
  }

  if (ERROR((rc = configure(d->db))))
    goto done;

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
