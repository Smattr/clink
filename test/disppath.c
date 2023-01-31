#include "../clink/src/path.h"
#include "test.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

TEST("test cases for clink/src/disppath.c:disppath()") {

  static const char target[] = "target";

  // construct a temporary directory
  char *path = test_mkdtemp();

  // change to this directory
  {
    int r = chdir(path);
    ASSERT_EQ(r, 0);
  }

  // create a temporary file within this temporary directory
  {
    FILE *f = fopen(target, "w");
    ASSERT_NOT_NULL(f);
    fprintf(f, "hello world\n");
    (void)fclose(f);
  }

  // standard disppath use case
  char *in1 = test_asprintf("%s/%s", path, target);
  const char *out1 = disppath(path, in1);
  bool claim1 = strcmp(out1, "target") == 0;

  // we should get an absolute path for something with a smaller common prefix
  const char *out2 = disppath(path, "/");
  bool claim2 = strcmp(out2, "/") == 0;

  // and we should get an absolute path for something with a different root
  const char *out3 = disppath(path, "/usr");
  bool claim3 = access("/usr", R_OK) != 0 || strcmp(out3, "/usr") == 0;

  // clean up temporary file
  (void)unlink(target);

  // assert all our claims
  ASSERT(claim1);
  ASSERT(claim2);
  ASSERT(claim3);
}
