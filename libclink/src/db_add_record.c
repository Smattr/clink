#include "db.h"
#include "debug.h"
#include "get_id.h"
#include "sql.h"
#include <clink/db.h>
#include <errno.h>
#include <sqlite3.h>
#include <stdint.h>
#include <string.h>

int clink_db_add_record(clink_db_t *db, const char *path, uint64_t hash,
                        uint64_t timestamp, clink_record_id_t *id) {

  if (ERROR(db == NULL))
    return EINVAL;

  if (ERROR(db->db == NULL))
    return EINVAL;

  if (ERROR(path == NULL))
    return EINVAL;

  if (ERROR(path[0] != '/'))
    return EINVAL;

  if (ERROR(strcmp(path, "") == 0))
    return EINVAL;

  static const char INSERT[] = "insert or replace into records (path, hash, "
                               "timestamp) values (@path, @hash, @timestamp);";

  int rc = 0;
  if (id != NULL)
    *id = -1;

  sqlite3_stmt *insert = NULL;
  if (ERROR((rc = sql_prepare(db->db, INSERT, &insert))))
    goto done;

  if (ERROR((rc = sql_bind_text(insert, 1, path))))
    goto done;

  if (ERROR((rc = sql_bind_int(insert, 2, hash))))
    goto done;

  if (ERROR((rc = sql_bind_int(insert, 3, timestamp))))
    goto done;

  {
    int r = sqlite3_step(insert);
    if (ERROR(r != SQLITE_DONE)) {
      rc = sql_err_to_errno(r);
      goto done;
    }
  }

  // if the user did not need the ID of this row, we are done
  if (id == NULL)
    goto done;

  // Now we need to look up the ID of the just-inserted record. We do this as a
  // separate query instead of using `sqlite3_last_insert_rowid` because
  // multiple threads or multiple processes may be operating on the database at
  // once.
  if (ERROR((rc = get_id(db, path, id))))
    goto done;

done:
  if (insert != NULL)
    sqlite3_finalize(insert);

  return rc;
}
