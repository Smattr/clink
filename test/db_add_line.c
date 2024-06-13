#include "test.h"
#include <clink/clink.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

TEST("clink_db_add_line()") {

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

  // add a record for this file
  static const char path[] = "/foo";
  {
    int rc = clink_db_add_record(db, path, 0, 0, NULL);
    ASSERT_EQ(rc, 0);
  }

  // add a new line of content
  {
    unsigned long lineno = 42;
    static const char line[] = "bar\n";

    int rc = clink_db_add_line(db, path, lineno, line);
    if (rc)
      fprintf(stderr, "clink_db_add_line: %s\n", strerror(rc));
    ASSERT_EQ(rc, 0);
  }

  // close the database
  clink_db_close(&db);
}
