#include <clink/db.h>
#include "db.h"
#include <errno.h>
#include "sql.h"
#include <sqlite3.h>
#include <stdint.h>
#include <string.h>

int clink_db_find_record(clink_db_t *db, const char *path, uint64_t *hash,
    uint64_t *timestamp) {

  if (db == NULL)
    return EINVAL;

  if (path == NULL)
    return EINVAL;

  if (strcmp(path, "") == 0)
    return EINVAL;

  static const char QUERY[]
    = "select hash, timestamp from records where path = @path;";

  int rc = 0;
  sqlite3_stmt *s = NULL;

  if ((rc = sql_prepare(db->db, QUERY, &s)))
    goto done;

  if ((rc = sql_bind_text(s, 1, path)))
    goto done;

  {
    int r = sqlite3_step(s);

    if (r != SQLITE_ROW) {
      if (r == SQLITE_DONE) {
        // no record found
        rc = ENOENT;
      } else {
        rc = sql_err_to_errno(r);
      }
      goto done;
    }
  }

  // extract the hash if the user requested it
  if (hash != NULL)
    *hash = sqlite3_column_int64(s, 0);

  // extract the timestamp if the user requested it
  if (timestamp != NULL)
    *timestamp = sqlite3_column_int64(s, 1);

done:
  if (s != NULL)
    sqlite3_finalize(s);

  return rc;
}
