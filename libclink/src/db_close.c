#include <clink/db.h>
#include "db.h"
#include <sqlite3.h>
#include <stdlib.h>

void clink_db_close(clink_db_t **db) {

  // allow freeing NULL
  if (db == NULL || *db == NULL)
    return;

  // close the database handle
  (void)sqlite3_close((*db)->db);

  free(*db);
  *db = NULL;
}
