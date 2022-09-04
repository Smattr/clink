#include "../libclink/src/run.h"
#include "test.h"
#include <stdlib.h>
#include <sys/stat.h>

TEST("running `touch filename` via libclink’s run() function does what we "
     "expect") {

  // construct a unique path to touch
  char *target = test_tmpnam();

  // use run() to touch the target
  const char *args[] = {"touch", target, NULL};
  {
    int r = run(args);
    ASSERT_EQ(r, EXIT_SUCCESS);
  }

  // read the target’s properties
  struct stat buf;
  int r = stat(target, &buf);

  // confirm the target did exist and was created empty
  ASSERT_EQ(r, 0);
  ASSERT_EQ(buf.st_size, 0);
}
