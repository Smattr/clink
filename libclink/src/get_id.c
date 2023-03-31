#include "get_id.h"
#include "db.h"
#include "debug.h"
#include "sql.h"
#include <assert.h>
#include <clink/db.h>
#include <errno.h>
#include <sqlite3.h>
#include <stddef.h>

int get_id(clink_db_t *db, const char *path, clink_record_id_t *id) {

  assert(db != NULL);
  assert(path != NULL);
  assert(id != NULL);

  int rc = 0;

  static const char LOOKUP[] = "select id from records where path = @path;";

  sqlite3_stmt *lookup = NULL;
  if (ERROR((rc = sql_prepare(db->db, LOOKUP, &lookup))))
    goto done;

  if (ERROR((rc = sql_bind_text(lookup, 1, path))))
    goto done;

  {
    int r = sqlite3_step(lookup);
    if (ERROR(r != SQLITE_ROW)) {
      if (r == SQLITE_DONE) {
        rc = ENOENT;
      } else {
        rc = sql_err_to_errno(r);
      }
      goto done;
    }
  }

  *id = sqlite3_column_int64(lookup, 0);

done:
  if (lookup != NULL)
    sqlite3_finalize(lookup);

  return rc;
}
