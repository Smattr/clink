#include "add_line.h"
#include "db.h"
#include "debug.h"
#include "get_id.h"
#include "sql.h"
#include <assert.h>
#include <clink/db.h>
#include <clink/vim.h>
#include <ctype.h>
#include <errno.h>
#include <sqlite3.h>
#include <stddef.h>
#include <string.h>

typedef struct {
  clink_db_t *db;
  clink_record_id_t path_id; ///< record identifier for source file
  unsigned long lineno;

  /// query for relevant line numbers of the file
  sqlite3_stmt *stmt;

  /// next relevant line number
  unsigned long next;
} state_t;

static int insert(void *state, char *line) {
  assert(state != NULL);
  assert(line != NULL);

  state_t *s = state;
  ++s->lineno;

  // find the next relevant line number if necessary
  if (s->stmt != NULL) {
    if (s->next == 0 || s->next < s->lineno) {
      int rc = sqlite3_step(s->stmt);
      if (rc == SQLITE_DONE) {
        sqlite3_finalize(s->stmt);
        s->stmt = NULL;
      } else if (ERROR(rc != SQLITE_ROW)) {
        return sql_err_to_errno(rc);
      } else {
        s->next = sqlite3_column_int64(s->stmt, 0);
        assert(s->next >= s->lineno && "unexpected symbol line number results");
      }
    }
  }

  // if this line is not relevant, skip it
  if (s->lineno != s->next)
    return 0;

  // eagerly strip leading white space from the line because we know the UI
  // wants it stripped eventually
  for (char *p = line; *p != '\0';) {
    if (isspace(*p)) {
      memmove(p, p + 1, strlen(p));
    } else if (*p == '\033') { // CSI
      // skip over it to the terminator
      for (char *q = p + 1; *q != '\0'; ++q) {
        if (*q == 'm') {
          p = q;
          break;
        }
      }
      ++p;
    } else { // non-white-space content
      break;
    }
  }

  return add_line(s->db, s->path_id, s->lineno, line);
}

int clink_vim_read_into(clink_db_t *db, const char *filename) {

  if (ERROR(db == NULL))
    return EINVAL;

  if (ERROR(filename == NULL))
    return EINVAL;

  if (ERROR(filename[0] != '/'))
    return EINVAL;

  int rc = 0;
  state_t s = {.db = db};

  // lookup the record identifier for the given path in advance
  clink_record_id_t id = -1;
  if (ERROR((rc = get_id(db, filename, &id))))
    goto done;
  s.path_id = id;

  // create a query to lookup relevant line numbers from the target file
  static const char QUERY[] =
      "select distinct line from symbols where path = @id order by line;";
  if (ERROR((rc = sql_prepare(db->db, QUERY, &s.stmt))))
    goto done;
  if (ERROR((rc = sql_bind_int(s.stmt, 1, id))))
    goto done;

  rc = clink_vim_read(filename, insert, &s);

done:
  if (s.stmt != NULL)
    sqlite3_finalize(s.stmt);

  return rc;
}
