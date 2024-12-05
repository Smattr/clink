#include "db.h"
#include "debug.h"
#include "iter.h"
#include "re.h"
#include "sql.h"
#include <clink/db.h>
#include <clink/iter.h>
#include <clink/symbol.h>
#include <errno.h>
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/// state for our iterator
typedef struct {

  /// database we are searching
  clink_db_t *db;

  /// the full regular expression pattern we are searching for
  char *pattern;

  /// SQL query we are executing
  sqlite3_stmt *stmt;

  /// last symbol we yielded
  clink_symbol_t last;

  /// a temporary buffer for storing resolved paths
  char *abs_path;
  size_t abs_path_size;

} state_t;

static void state_free(state_t **ss) {

  if (ss == NULL || *ss == NULL)
    return;

  state_t *s = *ss;

  s->abs_path_size = 0;
  free(s->abs_path);
  s->abs_path = NULL;

  s->last = (clink_symbol_t){0};

  if (s->stmt != NULL)
    sqlite3_finalize(s->stmt);
  s->stmt = NULL;

  free(s->pattern);
  s->pattern = NULL;

  free(s);
  *ss = NULL;
}

static int next(clink_iter_t *it, const clink_symbol_t **yielded) {

  if (ERROR(it == NULL))
    return EINVAL;

  if (ERROR(yielded == NULL))
    return EINVAL;

  state_t *s = it->state;

  // discard any previous symbol we had
  s->last = (clink_symbol_t){0};

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

  // construct a symbol from the result
  s->last.category = CLINK_ASSIGNMENT;
  s->last.name = (char *)sqlite3_column_text(s->stmt, 0);
  char *path = (char *)sqlite3_column_text(s->stmt, 1);
  if (path[0] == '/') {
    s->last.path = path;
  } else {
    if (s->abs_path_size < strlen(s->db->dir) + strlen(path) + 1) {
      size_t abs_path_size = strlen(s->db->dir) + strlen(path) + 1;
      char *a = realloc(s->abs_path, abs_path_size);
      if (ERROR(a == NULL))
        return ENOMEM;
      s->abs_path = a;
      if (s->abs_path_size == 0) // is this the first relative path?
        memcpy(s->abs_path, s->db->dir, strlen(s->db->dir));
      s->abs_path_size = abs_path_size;
    }
    memcpy(s->abs_path + strlen(s->db->dir), path, strlen(path) + 1);
    s->last.path = s->abs_path;
  }
  s->last.lineno = sqlite3_column_int64(s->stmt, 2);
  s->last.colno = sqlite3_column_int64(s->stmt, 3);
  s->last.start.lineno = sqlite3_column_int64(s->stmt, 4);
  s->last.start.colno = sqlite3_column_int64(s->stmt, 5);
  s->last.start.byte = sqlite3_column_int64(s->stmt, 6);
  s->last.end.lineno = sqlite3_column_int64(s->stmt, 7);
  s->last.end.colno = sqlite3_column_int64(s->stmt, 8);
  s->last.end.byte = sqlite3_column_int64(s->stmt, 9);
  s->last.parent = (char *)sqlite3_column_text(s->stmt, 10);
  s->last.context = (char *)sqlite3_column_text(s->stmt, 11);

  // yield it
  *yielded = &s->last;
  return 0;
}

static void my_free(clink_iter_t *it) {

  if (it == NULL)
    return;

  state_t *s = it->state;
  state_free(&s);
}

int clink_db_find_assignment(clink_db_t *db, const char *regex,
                             clink_iter_t **it) {

  if (ERROR(db == NULL))
    return EINVAL;

  if (ERROR(regex == NULL))
    return EINVAL;

  if (ERROR(it == NULL))
    return EINVAL;

  static const char QUERY[] =
      "select symbols.name, records.path, symbols.line, symbols.col, "
      "symbols.start_line, symbols.start_col, symbols.start_byte, "
      "symbols.end_line, symbols.end_col, symbols.end_byte, symbols.parent, "
      "content.body from symbols inner join records "
      "on symbols.path = records.id left join content on "
      "records.id = content.path and symbols.line = content.line where "
      "symbols.name regexp @name and symbols.category = @category order by "
      "records.path, symbols.line, symbols.col;";

  int rc = 0;
  clink_iter_t *i = NULL;

  // allocate state for our iterator
  state_t *s = calloc(1, sizeof(*s));
  if (ERROR(s == NULL)) {
    rc = ENOMEM;
    goto done;
  }
  s->db = db;

  // create a query to lookup assignments in the database
  if (ERROR((rc = sql_prepare(db->db, QUERY, &s->stmt))))
    goto done;

  // bind the where clause to our given definition
  if (ERROR(asprintf(&s->pattern, "^%s$", regex) < 0)) {
    rc = ENOMEM;
    goto done;
  }
  if (ERROR((rc = re_add(&db->regexes, s->pattern))))
    goto done;
  if (ERROR((rc = sql_bind_text(s->stmt, 1, s->pattern))))
    goto done;
  if (ERROR((rc = sql_bind_int(s->stmt, 2, CLINK_ASSIGNMENT))))
    goto done;

  // create an iterator for stepping through our query
  i = calloc(1, sizeof(*i));
  if (ERROR(i == NULL)) {
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
