#include "test.h"
#include <clink/db.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

TEST("clink_db_add_record()") {

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

  // add a new record
  {
    int rc = clink_db_add_record(db, "foo/bar.c", 42, 128);
    if (rc)
      fprintf(stderr, "clink_db_add_record: %s\n", strerror(rc));
    ASSERT_EQ(rc, 0);
  }

  // close the database
  clink_db_close(&db);
}
