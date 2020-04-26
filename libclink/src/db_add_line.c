#include <clink/db.h>
#include "db.h"
#include <errno.h>
#include "sql.h"
#include <sqlite3.h>
#include <stddef.h>

int clink_db_add_line(clink_db_t *db, const char *path, unsigned long lineno,
    const char *line) {

  if (db == NULL)
    return EINVAL;

  if (db->db == NULL)
    return EINVAL;

  if (path == NULL)
    return EINVAL;

  if (lineno == 0)
    return EINVAL;

  if (line == NULL)
    return EINVAL;

  // insert into the content table

  static const char CONTENT_INSERT[] = "insert or replace into content (path, "
    "line, body) values (@path, @line, @body);";

  int rc = 0;

  sqlite3_stmt *s = NULL;
  if ((rc = sql_prepare(db->db, CONTENT_INSERT, &s)))
    goto done;

  if ((rc = sql_bind_text(s, 1, path)))
    goto done;

  if ((rc = sql_bind_int(s, 2, lineno)))
    goto done;

  if ((rc = sql_bind_text(s, 3, line)))
    goto done;

  {
    int r = sqlite3_step(s);
    if (r != SQLITE_DONE) {
      rc = sql_err_to_errno(r);
      goto done;
    }
  }

done:
  if (s != NULL)
    sqlite3_finalize(s);

  return rc;
}
