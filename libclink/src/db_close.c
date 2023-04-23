#include "db.h"
#include "re.h"
#include <clink/db.h>
#include <pthread.h>
#include <sqlite3.h>
#include <stdbool.h>
#include <stdlib.h>

void clink_db_close(clink_db_t **db) {

  // allow freeing NULL
  if (db == NULL || *db == NULL)
    return;

  if ((*db)->bulk_operation_inited)
    (void)pthread_mutex_destroy(&(*db)->bulk_operation);
  (*db)->bulk_operation_inited = false;

  re_free(&(*db)->regexes);

  // close the database handle
  (void)sqlite3_close((*db)->db);

  free((*db)->filename);
  free((*db)->dir);

  free(*db);
  *db = NULL;
}
