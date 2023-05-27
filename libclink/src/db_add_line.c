#include "add_line.h"
#include "db.h"
#include "debug.h"
#include "get_id.h"
#include "sql.h"
#include <assert.h>
#include <clink/db.h>
#include <errno.h>
#include <sqlite3.h>
#include <stddef.h>

int add_line(clink_db_t *db, clink_record_id_t path, unsigned long lineno,
             const char *line) {

  assert(db != NULL);
  assert(path != -1);
  assert(lineno != 0);
  assert(line != NULL);

  int rc = 0;
  sqlite3_stmt *s = NULL;

  // insert into the content table

  static const char CONTENT_INSERT[] =
      "insert or replace into content (path, "
      "line, body) values (@path, @line, @body);";

  if (ERROR((rc = sql_prepare(db->db, CONTENT_INSERT, &s))))
    goto done;

  if (ERROR((rc = sql_bind_int(s, 1, path))))
    goto done;

  if (ERROR((rc = sql_bind_int(s, 2, lineno))))
    goto done;

  if (ERROR((rc = sql_bind_text(s, 3, line))))
    goto done;

  {
    int r = sqlite3_step(s);
    if (ERROR(r != SQLITE_DONE)) {
      rc = sql_err_to_errno(r);
      goto done;
    }
  }

done:
  if (s != NULL)
    sqlite3_finalize(s);

  return rc;
}

int clink_db_add_line(clink_db_t *db, const char *path, unsigned long lineno,
                      const char *line) {

  if (ERROR(db == NULL))
    return EINVAL;

  if (ERROR(db->db == NULL))
    return EINVAL;

  if (ERROR(path == NULL))
    return EINVAL;

  if (ERROR(path[0] != '/'))
    return EINVAL;

  if (ERROR(lineno == 0))
    return EINVAL;

  if (ERROR(line == NULL))
    return EINVAL;

  int rc = 0;

  // find the identifier for this line’s path
  clink_record_id_t id = -1;
  if (ERROR((rc = get_id(db, path, &id))))
    goto done;

  rc = add_line(db, id, lineno, line);

done:
  return rc;
}
