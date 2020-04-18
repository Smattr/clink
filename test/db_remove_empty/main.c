// test of clink_db_remove() on an empty database

// force assertions on
#ifdef NDEBUG
  #undef NDEBUG
#endif

#include <assert.h>
#include <clink/db.h>
#include <stdio.h>
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

  // remove on this empty database should work
  if (rc == 0)
    clink_db_remove(db, "hello-world");

  // close the database
  if (rc == 0)
    clink_db_close(&db);

  // clean up
  (void)unlink(target);
  (void)rmdir(path);

  // confirm that the database was opened correctly
  assert(rc == 0);

  return 0;
}
