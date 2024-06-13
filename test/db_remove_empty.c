#include "test.h"
#include <clink/clink.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

TEST("clink_db_remove() on an empty database") {

  // construct a unique path
  char *target = test_tmpnam();

  // open it as a database
  clink_db_t *db = NULL;
  {
    int rc = clink_db_open(&db, target);
    if (rc)
      fprintf(stderr, "clink_db_open: %s\n", strerror(rc));
    ASSERT_EQ(rc, 0);
  }

  // remove on this empty database should work
  clink_db_remove(db, "hello-world");

  // close the database
  clink_db_close(&db);
}
