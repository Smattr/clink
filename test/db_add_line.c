#include "test.h"
#include <clink/db.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

TEST("clink_db_add_line()") {

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

  // add a new symbol
  {
    static const char p[] = "foo";
    unsigned long lineno = 42;
    static const char line[] = "bar\n";

    int rc = clink_db_add_line(db, p, lineno, line);
    if (rc)
      fprintf(stderr, "clink_db_add_line: %s\n", strerror(rc));
    ASSERT_EQ(rc, 0);
  }

  // close the database
  clink_db_close(&db);
}
