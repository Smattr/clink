// a test that running `touch filename` via libclink’s run() function does what
// we expect

// force assertions on
#ifdef NDEBUG
  #undef NDEBUG
#endif

#include <assert.h>
#include "run.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
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

  // construct a path within the temporary directory to touch
  char *target = NULL;
  {
    int r = asprintf(&target, "%s/target", path);
    assert(r >= 0);
  }

  // use run() to touch the target
  const char *args[] = { "touch", target, NULL };
  {
    int r = run(args, false);
    assert(r == EXIT_SUCCESS);
  }

  // read the target’s properties
  struct stat buf;
  int r = stat(target, &buf);

  // clean up
  (void)unlink(target);
  (void)rmdir(path);

  // confirm the target did exist and was created empty
  assert(r == 0);
  assert(buf.st_size == 0);

  return 0;
}
