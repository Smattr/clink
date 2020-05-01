// basic test of clink_db_find_record()

// force assertions on
#ifdef NDEBUG
  #undef NDEBUG
#endif

#include <assert.h>
#include <clink/db.h>
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(void) {

  // find where we should be creating temporary files
  const char *tmp = getenv("TMPDIR");
  if (tmp == NULL)
    tmp = "/tmp";

  // construct a temporary path template
  char *path = NULL;
  {
    int r = asprintf(&path, "%s/tmp.XXXXXX", tmp);
    assert(r >= 0);
  }

  // create a temporary directory to work in
  {
    char *r = mkdtemp(path);
    assert(r != NULL);
  }

  // construct a path within the temporary directory
  char *target = NULL;
  {
    int r = asprintf(&target, "%s/target", path);
    assert(r >= 0);
  }

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
      fprintf(stderr, "unexpected result from finding non-existent file: %d != "
        "ENOENT\n", r);
      rc = -1;
    }
  }

  // the same, but with NULLs
  if (rc == 0) {
    uint64_t hash;
    int r = clink_db_find_record(db, "/baz/bar.c", &hash, NULL);
    if (r != ENOENT) {
      fprintf(stderr, "unexpected result from finding non-existent file, with "
        "null timestamp: %d != ENOENT\n", r);
      rc = -1;
    }
  }
  if (rc == 0) {
    uint64_t timestamp;
    int r = clink_db_find_record(db, "/baz/bar.c", NULL, &timestamp);
    if (r != ENOENT) {
      fprintf(stderr, "unexpected result from finding non-existent file, with "
        "null hash: %d != ENOENT\n", r);
      rc = -1;
    }
  }
  if (rc == 0) {
    int r = clink_db_find_record(db, "/baz/bar.c", NULL, NULL);
    if (r != ENOENT) {
      fprintf(stderr, "unexpected result from finding non-existent file, with "
        "null hash and timestamp: %d != ENOENT\n", r);
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

  // clean up
  (void)unlink(target);
  (void)rmdir(path);

  // confirm everything went correctly
  assert(rc == 0);

  return 0;
}
