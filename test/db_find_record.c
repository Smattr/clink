#include "test.h"
#include <clink/clink.h>
#include <errno.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

TEST("clink_db_find_record()") {

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

  // add a new record
  {
    int rc = clink_db_add_record(db, "/foo/bar.c", 42, 128, NULL);
    if (rc)
      fprintf(stderr, "clink_db_add_record: %s\n", strerror(rc));
    ASSERT_EQ(rc, 0);
  }

  // find something that does not exist
  {
    uint64_t hash, timestamp;
    int rc = clink_db_find_record(db, "/baz/bar.c", &hash, &timestamp);
    ASSERT_EQ(rc, ENOENT);
  }

  // the same, but with NULLs
  {
    uint64_t hash;
    int rc = clink_db_find_record(db, "/baz/bar.c", &hash, NULL);
    ASSERT_EQ(rc, ENOENT);
  }
  {
    uint64_t timestamp;
    int rc = clink_db_find_record(db, "/baz/bar.c", NULL, &timestamp);
    ASSERT_EQ(rc, ENOENT);
  }
  {
    int rc = clink_db_find_record(db, "/baz/bar.c", NULL, NULL);
    ASSERT_EQ(rc, ENOENT);
  }

  // now lookup an existing record
  {
    uint64_t hash, timestamp;
    int rc = clink_db_find_record(db, "/foo/bar.c", &hash, &timestamp);
    if (rc)
      fprintf(stderr, "unexpected result from finding file: %s\n",
              strerror(rc));
    ASSERT_EQ(rc, 0);

    ASSERT_EQ(hash, 42u);
    ASSERT_EQ(timestamp, 128u);
  }
  {
    uint64_t hash;
    int rc = clink_db_find_record(db, "/foo/bar.c", &hash, NULL);
    if (rc)
      fprintf(stderr, "unexpected result from finding file: %s\n",
              strerror(rc));
    ASSERT_EQ(rc, 0);

    ASSERT_EQ(hash, 42u);
  }
  {
    uint64_t timestamp;
    int rc = clink_db_find_record(db, "/foo/bar.c", NULL, &timestamp);
    if (rc)
      fprintf(stderr, "unexpected result from finding file: %d\n", rc);
    ASSERT_EQ(rc, 0);

    ASSERT_EQ(timestamp, 128u);
  }
  {
    int rc = clink_db_find_record(db, "/foo/bar.c", NULL, NULL);
    if (rc)
      fprintf(stderr, "unexpected result from finding file: %d\n", rc);
    ASSERT_EQ(rc, 0);
  }

  // close the database
  clink_db_close(&db);
}
