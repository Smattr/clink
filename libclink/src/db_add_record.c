#include "db.h"
#include "debug.h"
#include "sql.h"
#include <clink/db.h>
#include <errno.h>
#include <sqlite3.h>
#include <stdint.h>
#include <string.h>

int clink_db_add_record(clink_db_t *db, const char *path, uint64_t hash,
                        uint64_t timestamp) {

  if (ERROR(db == NULL))
    return EINVAL;

  if (ERROR(db->db == NULL))
    return EINVAL;

  if (ERROR(path == NULL))
    return EINVAL;

  if (ERROR(strcmp(path, "") == 0))
    return EINVAL;

  static const char INSERT[] = "insert or replace into records (path, hash, "
                               "timestamp) values (@path, @hash, @timestamp);";

  int rc = 0;

  sqlite3_stmt *s = NULL;
  if (ERROR((rc = sql_prepare(db->db, INSERT, &s))))
    goto done;

  if (ERROR((rc = sql_bind_text(s, 1, path))))
    goto done;

  if (ERROR((rc = sql_bind_int(s, 2, hash))))
    goto done;

  if (ERROR((rc = sql_bind_int(s, 3, timestamp))))
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
