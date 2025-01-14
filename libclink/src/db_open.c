#include "../../common/compiler.h"
#include "db.h"
#include "debug.h"
#include "re.h"
#include "schema.h"
#include "sql.h"
#include <assert.h>
#include <clink/db.h>
#include <errno.h>
#include <inttypes.h>
#include <pthread.h>
#include <sqlite3.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/// shim `sqlite3_error_offset` which was introduced in 3.38.0
#if SQLITE_VERSION_NUMBER < 3 * 1000000 + 38 * 1000 + 0
static int sqlite3_error_offset(sqlite3 *db UNUSED) { return -1; }
#endif

/// debug-dump extra details about a SQLite error that just occurred
#define SQL_ERROR_DETAIL(db, query)                                            \
  do {                                                                         \
    if (UNLIKELY(clink_debug != NULL)) {                                       \
      const int errcode_ = sqlite3_errcode(db);                                \
      const int extended_ = sqlite3_extended_errcode(db);                      \
      const int offset_ = sqlite3_error_offset(db);                            \
      const char *msg_ = sqlite3_errmsg(db);                                   \
      DEBUG(                                                                   \
          "SQLite error %d, extended error %d: “%s” at character %d of “%s”",  \
          errcode_, extended_, msg_, offset_, (query));                        \
    }                                                                          \
  } while (0)

/// application identifier to write into the corresponding SQLite header field
///
/// In theory, this information can be used by tools like `file` to give more
/// accurate information about file type. For us, we just use it as a sanity
/// check that we are actually opening a database created by ourselves. The
/// value is entirely arbitrary.
#define APP_ID 0x6e696c63 // “clin” in big endian

/// version of schema.sql
///
/// This needs to be bumped whenever the database schema changes to avoid
/// running queries against mismatched table structures. This includes when a
/// functional change is made that does not affect the structural identity of
/// the database tables but impacts backward/forward compatibility.
#define SCHEMA_VERSION 1

#define STR_(x) #x
#define STR(x) STR_(x)

const char *schema_version(void) { return STR(SCHEMA_VERSION); }

static int exec_all(sqlite3 *db, size_t queries_length, const char **queries) {

  assert(db != NULL);
  assert(queries_length == 0 || queries != NULL);

  int rc = 0;

  for (size_t i = 0; i < queries_length; ++i) {
    assert(queries[i] != NULL);
    if (ERROR((rc = sql_exec(db, queries[i])))) {
      SQL_ERROR_DETAIL(db, queries[i]);
      return rc;
    }
  }

  return rc;
}

static int init(sqlite3 *db) {
  assert(db != NULL);

  int rc = 0;

  // setup the schema
  if (ERROR((rc = exec_all(db, SCHEMA_LENGTH, SCHEMA))))
    goto done;

  // write our identifying version information
  static const char SET_APP_ID[] = "pragma application_id = " STR(APP_ID) ";";
  if (ERROR((rc = sql_exec(db, SET_APP_ID)))) {
    SQL_ERROR_DETAIL(db, SET_APP_ID);
    goto done;
  }
  static const char SET_VER[] =
      "pragma user_version = " STR(SCHEMA_VERSION) ";";
  if (ERROR((rc = sql_exec(db, SET_VER)))) {
    SQL_ERROR_DETAIL(db, SET_VER);
    goto done;
  }

done:
  return rc;
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

  sqlite3_stmt *get_app_id = NULL;
  sqlite3_stmt *get_ver = NULL;
  int rc = 0;

  // lookup the application ID
  static const char GET_APP_ID[] = "pragma application_id;";
  {
    const int r = sqlite3_prepare_v2(db, GET_APP_ID, sizeof(GET_APP_ID),
                                     &get_app_id, NULL);
    if (ERROR(r)) {
      SQL_ERROR_DETAIL(db, GET_APP_ID);
      rc = sql_err_to_errno(r);
      goto done;
    }
  }

  {
    const int r = sqlite3_step(get_app_id);
    if (ERROR(r != SQLITE_ROW)) {
      if (r == SQLITE_DONE) {
        rc = ENOENT;
      } else {
        SQL_ERROR_DETAIL(db, GET_APP_ID);
        rc = sql_err_to_errno(r);
      }
      goto done;
    }
  }

  // is the SQLite application ID what we expect?
  const uint32_t app_id = sqlite3_column_int64(get_app_id, 0);
  if (app_id != APP_ID) {
    DEBUG("expected SQLite application ID 0x%" PRIx32 " but saw 0x%" PRIx32,
          (uint32_t)APP_ID, app_id);
    rc = EPROTO;
    goto done;
  }

  // lookup the user version
  static const char GET_VER[] = "pragma user_version;";
  {
    const int r =
        sqlite3_prepare_v2(db, GET_VER, sizeof(GET_VER), &get_ver, NULL);
    if (ERROR(r)) {
      SQL_ERROR_DETAIL(db, GET_VER);
      rc = sql_err_to_errno(r);
      goto done;
    }
  }

  {
    const int r = sqlite3_step(get_ver);
    if (ERROR(r != SQLITE_ROW)) {
      if (r == SQLITE_DONE) {
        rc = ENOENT;
      } else {
        SQL_ERROR_DETAIL(db, GET_VER);
        rc = sql_err_to_errno(r);
      }
      goto done;
    }
  }

  // is the SQLite user version what we expect?
  const uint32_t ver = sqlite3_column_int64(get_ver, 0);
  if (ver != SCHEMA_VERSION) {
    DEBUG("expection SQLite user version 0x%" PRIx32 " but saw 0x%" PRIx32,
          (uint32_t)SCHEMA_VERSION, ver);
    rc = EPROTO;
    goto done;
  }

done:
  if (get_app_id != NULL)
    sqlite3_finalize(get_app_id);
  if (get_ver != NULL)
    sqlite3_finalize(get_ver);

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

  {
    const int err = sqlite3_open(path, &d->db);
    if (err != SQLITE_OK) {
      rc = sql_err_to_errno(err);
      goto done;
    }
  }

  // disable double-quotes for string literals in CREATE
  {
    const int err = sqlite3_db_config(d->db, SQLITE_DBCONFIG_DQS_DDL, 0, NULL);
    if (ERROR(err != SQLITE_OK)) {
      SQL_ERROR_DETAIL(d->db, ".dbconfig DQS_DDL 0");
      rc = sql_err_to_errno(err);
      goto done;
    }
  }

  // disable double-quotes for string literals in DELETE/INSERT/SELECT/UPDATE
  {
    const int err = sqlite3_db_config(d->db, SQLITE_DBCONFIG_DQS_DML, 0, NULL);
    if (ERROR(err != SQLITE_OK)) {
      SQL_ERROR_DETAIL(d->db, ".dbconfig DQS_DML 0");
      rc = sql_err_to_errno(err);
      goto done;
    }
  }

  // now that the database exists, find an absolute path to it
  if (path[0] != '/') {
    char *abs = realpath(path, NULL);
    if (ERROR(abs == NULL)) {
      rc = errno;
      goto done;
    }
    char *slash = strrchr(abs, '/');
    assert(slash != NULL);
    d->filename = strdup(slash + 1);
    slash[1] = '\0';
    d->dir = abs;
  } else {
    char *slash = strrchr(path, '/');
    d->dir = strndup(path, (size_t)(slash + 1 - path));
    d->filename = strdup(slash + 1);
  }
  if (ERROR(d->dir == NULL)) {
    rc = ENOMEM;
    goto done;
  }
  if (ERROR(d->filename == NULL)) {
    rc = ENOMEM;
    goto done;
  }

  if (exists) {
    if (ERROR((rc = check_schema_version(d->db))))
      goto done;
  } else {
    if (ERROR((rc = init(d->db))))
      goto done;
    assert(check_schema_version(d->db) == 0 &&
           "a database we created claims to have been created by a different "
           "Clink version");
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
