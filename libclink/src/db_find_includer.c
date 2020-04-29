#include <clink/db.h>
#include <clink/iter.h>
#include <clink/symbol.h>
#include "db.h"
#include <errno.h>
#include "iter.h"
#include "sql.h"
#include <sqlite3.h>
#include <stdlib.h>
#include <string.h>

/// state for our iterator
typedef struct {

  /// the name of the file we are searching for #includes of
  char *name;

  /// SQL query we are executing
  sqlite3_stmt *stmt;

  /// last symbol we yielded
  clink_symbol_t last;

} state_t;

static void state_free(state_t **ss) {

  if (ss == NULL || *ss == NULL)
    return;

  state_t *s = *ss;

  memset(&s->last, 0, sizeof(s->last));

  if (s->stmt != NULL)
    sqlite3_finalize(s->stmt);
  s->stmt = NULL;

  free(s->name);
  s->name = NULL;

  free(s);
  *ss = NULL;
}

static int next(no_lookahead_iter_t *it, const clink_symbol_t **yielded) {

  if (it == NULL)
    return EINVAL;

  if (yielded == NULL)
    return EINVAL;

  state_t *s = it->state;

  // discard any previous symbol we had
  memset(&s->last, 0, sizeof(s->last));

  // is the iterator exhausted?
  if (s->stmt == NULL)
    return ENOMSG;

  // extract the next result
  int rc = sqlite3_step(s->stmt);
  if (rc != SQLITE_ROW && rc != SQLITE_DONE)
    return sql_err_to_errno(rc);

  // did we just exhaust this iterator?
  if (rc == SQLITE_DONE) {
    sqlite3_finalize(s->stmt);
    s->stmt = NULL;
    return ENOMSG;
  }

  // construct a symbol from the result
  s->last.category = CLINK_INCLUDE;
  s->last.name = s->name;
  s->last.path = (char*)sqlite3_column_text(s->stmt, 0);
  s->last.lineno = sqlite3_column_int64(s->stmt, 1);
  s->last.colno = sqlite3_column_int64(s->stmt, 2);
  s->last.parent = (char*)sqlite3_column_text(s->stmt, 3);
  s->last.context = (char*)sqlite3_column_text(s->stmt, 4);

  // yield it
  *yielded = &s->last;
  return 0;
}

static void my_free(no_lookahead_iter_t *it) {

  if (it == NULL)
    return;

  state_free((state_t**)&it->state);
}

int clink_db_find_includer(clink_db_t *db, const char *name,
    clink_iter_t **it) {

  if (db == NULL)
    return EINVAL;

  if (name == NULL)
    return EINVAL;

  if (strcmp(name, "") == 0)
    return EINVAL;

  if (it == NULL)
    return EINVAL;

  static const char QUERY[] = "select symbols.path, symbols.line, symbols.col, "
    "symbols.parent, content.body from symbols left join content on "
    "symbols.path = content.path and symbols.line = content.line where "
    "symbols.name = @name and symbols.category = @category;";

  int rc = 0;
  no_lookahead_iter_t *i = NULL;
  clink_iter_t *wrapper = NULL;

  // allocate state for our iterator
  state_t *s = calloc(1, sizeof(*s));
  if (s == NULL) {
    rc = ENOMEM;
    goto done;
  }

  // save the name of the #include file for later symbol construction
  s->name = strdup(name);
  if (s->name == NULL) {
    rc = ENOMEM;
    goto done;
  }

  // create a query to lookup #includes in the database
  if ((rc = sql_prepare(db->db, QUERY, &s->stmt)))
    goto done;

  // bind the where clause to our given function
  if ((rc = sql_bind_text(s->stmt, 1, s->name)))
    goto done;
  if ((rc = sql_bind_int(s->stmt, 2, CLINK_INCLUDE)))
    goto done;

  // create a no-lookahead iterator for stepping through our query
  i = calloc(1, sizeof(*i));
  if (i == NULL) {
    rc = ENOMEM;
    goto done;
  }

  // configure it to iterate through our query
  i->next_symbol = next;
  i->state = s;
  s = NULL;
  i->free = my_free;

  // create a 1-lookahead iterator to wrap it
  if ((rc = iter_new(&wrapper, i)))
    goto done;
  i = NULL;

done:
  if (rc) {
    clink_iter_free(&wrapper);
    no_lookahead_iter_free(&i);
    state_free(&s);
  } else {
    *it = wrapper;
  }

  return rc;
}

