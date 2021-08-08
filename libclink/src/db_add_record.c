#include <clink/db.h>
#include "../../common/compiler.h"
#include "db.h"
#include <errno.h>
#include "sql.h"
#include <sqlite3.h>
#include <stdint.h>
#include <string.h>

int clink_db_add_record(clink_db_t *db, const char *path, uint64_t hash,
    uint64_t timestamp) {

  if (UNLIKELY(db == NULL))
    return EINVAL;

  if (UNLIKELY(db->db == NULL))
    return EINVAL;

  if (UNLIKELY(path == NULL))
    return EINVAL;

  if (UNLIKELY(strcmp(path, "") == 0))
    return EINVAL;

  static const char INSERT[] = "insert or replace into records (path, hash, "
    "timestamp) values (@path, @hash, @timestamp);";

  int rc = 0;

  sqlite3_stmt *s = NULL;
  if (UNLIKELY((rc = sql_prepare(db->db, INSERT, &s))))
    goto done;

  if (UNLIKELY((rc = sql_bind_text(s, 1, path))))
    goto done;

  if (UNLIKELY((rc = sql_bind_int(s, 2, hash))))
    goto done;

  if (UNLIKELY((rc = sql_bind_int(s, 3, timestamp))))
    goto done;

  {
    int r = sqlite3_step(s);
    if (UNLIKELY(r != SQLITE_DONE)) {
      rc = sql_err_to_errno(r);
      goto done;
    }
  }

done:
  if (s != NULL)
    sqlite3_finalize(s);

  return rc;
}
