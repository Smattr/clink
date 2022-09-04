#include "test.h"
#include <clink/db.h>
#include <errno.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

TEST("clink_db_find_record()") {

  // construct a unique path
  char *target = test_tmpnam();

  // open it as a database
  clink_db_t *db = NULL;
  int rc = clink_db_open(&db, target);
  if (rc)
    fprintf(stderr, "clink_db_open: %s\n", strerror(rc));

  // add a new record
  if (rc == 0) {
    if ((rc = clink_db_add_record(db, "/foo/bar.c", 42, 128)))
      fprintf(stderr, "clink_db_add_record: %s\n", strerror(rc));
  }

  // find something that does not exist
  if (rc == 0) {
    uint64_t hash, timestamp;
    int r = clink_db_find_record(db, "/baz/bar.c", &hash, &timestamp);
    if (r != ENOENT) {
      fprintf(stderr,
              "unexpected result from finding non-existent file: %d != "
              "ENOENT\n",
              r);
      rc = -1;
    }
  }

  // the same, but with NULLs
  if (rc == 0) {
    uint64_t hash;
    int r = clink_db_find_record(db, "/baz/bar.c", &hash, NULL);
    if (r != ENOENT) {
      fprintf(stderr,
              "unexpected result from finding non-existent file, with "
              "null timestamp: %d != ENOENT\n",
              r);
      rc = -1;
    }
  }
  if (rc == 0) {
    uint64_t timestamp;
    int r = clink_db_find_record(db, "/baz/bar.c", NULL, &timestamp);
    if (r != ENOENT) {
      fprintf(stderr,
              "unexpected result from finding non-existent file, with "
              "null hash: %d != ENOENT\n",
              r);
      rc = -1;
    }
  }
  if (rc == 0) {
    int r = clink_db_find_record(db, "/baz/bar.c", NULL, NULL);
    if (r != ENOENT) {
      fprintf(stderr,
              "unexpected result from finding non-existent file, with "
              "null hash and timestamp: %d != ENOENT\n",
              r);
      rc = -1;
    }
  }

  // now lookup an existing record
  if (rc == 0) {
    do {
      uint64_t hash, timestamp;
      if ((rc = clink_db_find_record(db, "/foo/bar.c", &hash, &timestamp))) {
        fprintf(stderr, "unexpected result from finding file: %s\n",
                strerror(rc));
        break;
      }

      if (hash != 42) {
        fprintf(stderr, "unexpected hash: %" PRIu64 "\n", hash);
        rc = -1;
        break;
      }

      if (timestamp != 128) {
        fprintf(stderr, "unexpected timestamp: %" PRIu64 "\n", timestamp);
        rc = -1;
        break;
      }
    } while (0);
  }
  if (rc == 0) {
    do {
      uint64_t hash;
      if ((rc = clink_db_find_record(db, "/foo/bar.c", &hash, NULL))) {
        fprintf(stderr, "unexpected result from finding file: %s\n",
                strerror(rc));
        break;
      }

      if (hash != 42) {
        fprintf(stderr, "unexpected hash: %" PRIu64 "\n", hash);
        rc = -1;
        break;
      }
    } while (0);
  }
  if (rc == 0) {
    do {
      uint64_t timestamp;
      if ((rc = clink_db_find_record(db, "/foo/bar.c", NULL, &timestamp))) {
        fprintf(stderr, "unexpected result from finding file: %d\n", rc);
        break;
      }

      if (timestamp != 128) {
        fprintf(stderr, "unexpected timestamp: %" PRIu64 "\n", timestamp);
        rc = -1;
        break;
      }
    } while (0);
  }
  if (rc == 0) {
    if ((rc = clink_db_find_record(db, "/foo/bar.c", NULL, NULL)))
      fprintf(stderr, "unexpected result from finding file: %d\n", rc);
  }

  // close the database
  if (rc == 0)
    clink_db_close(&db);

  // confirm everything went correctly
  ASSERT_EQ(rc, 0);
}
