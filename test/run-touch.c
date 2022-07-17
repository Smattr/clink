#include "../libclink/src/run.h"
#include "test.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

TEST("running `touch filename` via libclink’s run() function does what we "
     "expect") {

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

  // construct a path within the temporary directory to touch
  char *target = NULL;
  {
    int r = asprintf(&target, "%s/target", path);
    ASSERT_GE(r, 0);
  }

  // use run() to touch the target
  const char *args[] = {"touch", target, NULL};
  {
    int r = run(args);
    ASSERT_EQ(r, EXIT_SUCCESS);
  }

  // read the target’s properties
  struct stat buf;
  int r = stat(target, &buf);

  // clean up
  (void)unlink(target);
  free(target);
  (void)rmdir(path);
  free(path);

  // confirm the target did exist and was created empty
  ASSERT_EQ(r, 0);
  ASSERT_EQ(buf.st_size, 0);
}
