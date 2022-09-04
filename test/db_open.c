#include "test.h"
#include <clink/db.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

TEST("clink_db_open()") {

  // construct a unique path
  char *target = test_tmpnam();

  // open it as a database
  clink_db_t *db = NULL;
  int rc = clink_db_open(&db, target);
  if (rc)
    fprintf(stderr, "clink_db_open: %s\n", strerror(rc));

  // close the database
  if (rc == 0)
    clink_db_close(&db);

  // confirm that the database was opened correctly
  ASSERT_EQ(rc, 0);
}
