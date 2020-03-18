#include <assert.h>
#include <clink/db.h>
#include <sqlite3.h>
#include <stddef.h>

void clink_db_close(struct clink_db *db) {
  assert(db != NULL);
  sqlite3_close(db->handle);
}
