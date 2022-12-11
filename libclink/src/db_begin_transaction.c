#include "db.h"
#include "debug.h"
#include "sql.h"
#include <clink/db.h>
#include <errno.h>
#include <stddef.h>

int clink_db_begin_transaction(clink_db_t *db) {

  if (ERROR(db == NULL))
    return EINVAL;

  return sql_exec(db->db, "begin immediate;");
}
