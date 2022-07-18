#include "test.h"
#include <clink/db.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

TEST("clink_db_add_record()") {

  // find where we should be creating temporary files
  const char *tmp = getenv("TMPDIR");
  if (tmp == NULL)
    tmp = "/tmp";

  // construct a temporary path template
  char *path = NULL;
  {
    int r = asprintf(&path, "%s/tmp.XXXXXX", tmp);
    ASSERT_GE(r, 0);
  }

  // create a temporary directory to work in
  {
    char *r = mkdtemp(path);
    ASSERT_NOT_NULL(r);
  }

  // construct a path within the temporary directory
  char *target = NULL;
  {
    int r = asprintf(&target, "%s/target", path);
    ASSERT_GE(r, 0);
  }

  // open it as a database
  clink_db_t *db = NULL;
  int rc = clink_db_open(&db, target);
  if (rc)
    fprintf(stderr, "clink_db_open: %s\n", strerror(rc));

  // add a new record
  if (rc == 0) {
    if ((rc = clink_db_add_record(db, "foo/bar.c", 42, 128)))
      fprintf(stderr, "clink_db_add_record: %s\n", strerror(rc));
  }

  // close the database
  if (rc == 0)
    clink_db_close(&db);

  // clean up
  (void)unlink(target);
  free(target);
  (void)rmdir(path);
  free(path);

  // confirm that the database was opened correctly
  ASSERT_EQ(rc, 0);
}