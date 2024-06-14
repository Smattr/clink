#include "test.h"
#include <clink/clink.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

TEST("addition using relative paths should be rejected") {

  (void)clink_set_debug(stderr);

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

  // try to add new records with relative paths
  {
    int rc = clink_db_add_record(db, "foo", 0, 0, NULL);
    ASSERT_NE(rc, 0);
  }
  {
    int rc = clink_db_add_record(db, "./foo", 0, 0, NULL);
    ASSERT_NE(rc, 0);
  }
  {
    int rc = clink_db_add_record(db, "../foo", 0, 0, NULL);
    ASSERT_NE(rc, 0);
  }
  {
    int rc = clink_db_add_record(db, "foo/bar", 0, 0, NULL);
    ASSERT_NE(rc, 0);
  }

  // try to add content with relative paths
  {
    int rc = clink_db_add_line(db, "foo", 42, "bar");
    ASSERT_NE(rc, 0);
  }
  {
    int rc = clink_db_add_line(db, "./foo", 42, "bar");
    ASSERT_NE(rc, 0);
  }
  {
    int rc = clink_db_add_line(db, "../foo", 42, "bar");
    ASSERT_NE(rc, 0);
  }
  {
    int rc = clink_db_add_line(db, "foo/bar", 42, "bar");
    ASSERT_NE(rc, 0);
  }

  // close the database
  clink_db_close(&db);
}
