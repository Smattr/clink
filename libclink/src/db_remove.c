#include "db.h"
#include "debug.h"
#include "get_id.h"
#include "sql.h"
#include <clink/db.h>
#include <sqlite3.h>
#include <stddef.h>

void clink_db_remove(clink_db_t *db, const char *path) {

  if (ERROR(db == NULL))
    return;

  if (ERROR(db->db == NULL))
    return;

  if (ERROR(path == NULL))
    return;

  // find the record identifier for this path
  clink_record_id_t id = -1;
  {
    int r = get_id(db, path, &id);
    if (r == ENOENT) {
      // this path already does not exist in the database
      return;
    }
    // if something else went wrong, give up
    if (ERROR(r != 0))
      return;
  }

  // delete the path from the symbols table
  {
    static const char SYMBOLS_DELETE[] =
        "delete from symbols where path = @path";

    sqlite3_stmt *s = NULL;
    if (ERROR(sql_prepare(db->db, SYMBOLS_DELETE, &s)))
      return;

    if (ERROR(sql_bind_int(s, 1, id))) {
      sqlite3_finalize(s);
      return;
    }

    (void)sqlite3_step(s);

    sqlite3_finalize(s);
  }

  // now delete it from the content table
  {
    static const char CONTENT_DELETE[] =
        "delete from content where path = @path";

    sqlite3_stmt *s = NULL;
    if (ERROR(sql_prepare(db->db, CONTENT_DELETE, &s)))
      return;

    if (ERROR(sql_bind_text(s, 1, path))) {
      sqlite3_finalize(s);
      return;
    }

    (void)sqlite3_step(s);

    sqlite3_finalize(s);
  }

  // now delete it from the record table
  {
    static const char DELETE[] = "delete from records where path = @path";

    sqlite3_stmt *s = NULL;
    if (ERROR(sql_prepare(db->db, DELETE, &s)))
      return;

    if (ERROR(sql_bind_text(s, 1, path))) {
      sqlite3_finalize(s);
      return;
    }

    (void)sqlite3_step(s);

    sqlite3_finalize(s);
  }
}
