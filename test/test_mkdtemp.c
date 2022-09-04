#include "test.h"
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>

static void my_rmdir(void *arg) {
  assert(arg != NULL);
  assert(access(arg, F_OK) == 0);

  // in case we are inside the directory, move elsewhere so we can remove it
  (void)chdir("/");

  (void)rmdir(arg);
}

char *test_mkdtemp(void) {

  // find where we should be creating temporary files
  const char *tmp = getenv("TMPDIR");
  if (tmp == NULL)
    tmp = "/tmp";

  // construct a temporary path template
  char *path = test_asprintf("%s/tmp.XXXXXX", tmp);

  // create the temporary directory
  {
    char *r = mkdtemp(path);
    ASSERT_NOT_NULL(r);
  }

  // allocate a new cleanup action
  cleanup_t *c = calloc(1, sizeof(*c));
  if (c == NULL)
    rmdir(path);
  ASSERT_NOT_NULL(c);

  // set it up to delete the directory we just created
  c->function = my_rmdir;
  c->arg = path;

  // register it
  c->next = cleanups;
  cleanups = c;

  return path;
}
