#include "db.h"
#include "debug.h"
#include "iter.h"
#include "sql.h"
#include <clink/db.h>
#include <clink/iter.h>
#include <errno.h>
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/// state for our iterator
typedef struct {

  /// SQL query we are executing
  sqlite3_stmt *stmt;

  /// last filename we yielded
  char *last;

} state_t;

static void state_free(state_t **ss) {

  if (ss == NULL || *ss == NULL)
    return;

  state_t *s = *ss;

  if (s->stmt != NULL)
    sqlite3_finalize(s->stmt);
  s->stmt = NULL;

  free(s);
  *ss = NULL;
}

static int next(clink_iter_t *it, const char **yielded) {

  if (ERROR(it == NULL))
    return EINVAL;

  if (ERROR(yielded == NULL))
    return EINVAL;

  state_t *s = it->state;

  // is the iterator exhausted?
  if (s->stmt == NULL)
    return ENOMSG;

  // extract the next result
  int rc = sqlite3_step(s->stmt);
  if (ERROR(rc != SQLITE_ROW && rc != SQLITE_DONE))
    return sql_err_to_errno(rc);

  // did we just exhaust this iterator?
  if (rc == SQLITE_DONE) {
    sqlite3_finalize(s->stmt);
    s->stmt = NULL;
    return ENOMSG;
  }

  // extract the path
  s->last = (char *)sqlite3_column_text(s->stmt, 0);

  // yield it
  *yielded = s->last;
  return 0;
}

static void my_free(clink_iter_t *it) {

  if (it == NULL)
    return;

  state_t *s = it->state;
  state_free(&s);
}

int clink_db_find_file(clink_db_t *db, const char *name, clink_iter_t **it) {

  if (ERROR(db == NULL))
    return EINVAL;

  if (ERROR(name == NULL))
    return EINVAL;

  if (ERROR(strcmp(name, "") == 0))
    return EINVAL;

  if (ERROR(it == NULL))
    return EINVAL;

  static const char QUERY[] = "select distinct path from symbols where path = "
                              "@path1 or path like @path2 order by path;";

  int rc = 0;
  clink_iter_t *i = NULL;

  // allocate state for our iterator
  state_t *s = calloc(1, sizeof(*s));
  if (ERROR(s == NULL)) {
    rc = ENOMEM;
    goto done;
  }

  // create a query to lookup calls in the database
  if (ERROR((rc = sql_prepare(db->db, QUERY, &s->stmt))))
    goto done;

  // bind the where clause to our given function
  if (ERROR((rc = sqlite3_bind_text(s->stmt, 1, name, -1, SQLITE_TRANSIENT)))) {
    rc = sql_err_to_errno(rc);
    goto done;
  }
  {
    char *name2 = NULL;
    if (ERROR(asprintf(&name2, "%%/%s", name) < 0)) {
      rc = ENOMEM;
      goto done;
    }
    if (ERROR((rc = sqlite3_bind_text(s->stmt, 2, name2, -1, free)))) {
      rc = sql_err_to_errno(rc);
      goto done;
    }
  }

  // create an iterator for stepping through our query
  i = calloc(1, sizeof(*i));
  if (ERROR(i == NULL)) {
    rc = ENOMEM;
    goto done;
  }

  // configure it to iterate through our query
  i->next_str = next;
  i->state = s;
  s = NULL;
  i->free = my_free;

done:
  if (rc) {
    clink_iter_free(&i);
    state_free(&s);
  } else {
    *it = i;
  }

  return rc;
}
