#include <clink/db.h>
#include <clink/iter.h>
#include <clink/symbol.h>
#include "../../common/compiler.h"
#include "db.h"
#include <errno.h>
#include "iter.h"
#include "sql.h"
#include <sqlite3.h>
#include <stdlib.h>
#include <string.h>

/// state for our iterator
typedef struct {

  /// the name of the symbol we are searching for
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

static int next(clink_iter_t *it, const clink_symbol_t **yielded) {

  if (UNLIKELY(it == NULL))
    return EINVAL;

  if (UNLIKELY(yielded == NULL))
    return EINVAL;

  state_t *s = it->state;

  // discard any previous symbol we had
  memset(&s->last, 0, sizeof(s->last));

  // is the iterator exhausted?
  if (s->stmt == NULL)
    return ENOMSG;

  // extract the next result
  int rc = sqlite3_step(s->stmt);
  if (UNLIKELY(rc != SQLITE_ROW && rc != SQLITE_DONE))
    return sql_err_to_errno(rc);

  // did we just exhaust this iterator?
  if (rc == SQLITE_DONE) {
    sqlite3_finalize(s->stmt);
    s->stmt = NULL;
    return ENOMSG;
  }

  // construct a symbol from the result
  s->last.category = sqlite3_column_int64(s->stmt, 1);
  s->last.name = s->name;
  s->last.path = (char*)sqlite3_column_text(s->stmt, 0);
  s->last.lineno = sqlite3_column_int64(s->stmt, 2);
  s->last.colno = sqlite3_column_int64(s->stmt, 3);
  s->last.parent = (char*)sqlite3_column_text(s->stmt, 4);
  s->last.context = (char*)sqlite3_column_text(s->stmt, 5);

  // yield it
  *yielded = &s->last;
  return 0;
}

static void my_free(clink_iter_t *it) {

  if (it == NULL)
    return;

  state_free((state_t**)&it->state);
}

int clink_db_find_symbol(clink_db_t *db, const char *name, clink_iter_t **it) {

  if (UNLIKELY(db == NULL))
    return EINVAL;

  if (UNLIKELY(name == NULL))
    return EINVAL;

  if (UNLIKELY(strcmp(name, "") == 0))
    return EINVAL;

  if (UNLIKELY(it == NULL))
    return EINVAL;

  static const char QUERY[] = "select symbols.path, symbols.category, "
    "symbols.line, symbols.col, symbols.parent, content.body from symbols left "
    "join content on symbols.path = content.path and symbols.line = "
    "content.line where symbols.name = @name order by symbols.path, "
    "symbols.line, symbols.col;";

  int rc = 0;
  clink_iter_t *i = NULL;

  // allocate state for our iterator
  state_t *s = calloc(1, sizeof(*s));
  if (UNLIKELY(s == NULL)) {
    rc = ENOMEM;
    goto done;
  }

  // save the name for later symbol construction
  s->name = strdup(name);
  if (UNLIKELY(s->name == NULL)) {
    rc = ENOMEM;
    goto done;
  }

  // create a query to lookup the symbol in the database
  if (UNLIKELY((rc = sql_prepare(db->db, QUERY, &s->stmt))))
    goto done;

  if (UNLIKELY((rc = sql_bind_text(s->stmt, 1, s->name))))
    goto done;

  // create an iterator for stepping through this query
  i = calloc(1, sizeof(*i));
  if (UNLIKELY(i == NULL)) {
    rc = ENOMEM;
    goto done;
  }

  // configure it to iterate through our query
  i->next_symbol = next;
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
