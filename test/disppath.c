#include "../clink/src/path.h"
#include "test.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

TEST("test cases for clink/src/disppath.c:disppath()") {

  static const char target[] = "target";

  // change into the root to give us a statically known cwd
  {
    int r = chdir("/");
    ASSERT_EQ(r, 0);
  }

  // disppath with invalid parameters should fail
  {
    char *out = NULL;
    ASSERT_NE(disppath(NULL, "/usr", &out), 0);
    ASSERT_NE(disppath("/", NULL, &out), 0);
    ASSERT_NE(disppath("/", target, NULL), 0);
  }

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
  char *out1 = NULL;
  int r1 = disppath(path, in1, &out1);
  bool claim1_a = r1 == 0;
  bool claim1_b = claim1_a && strcmp(out1, "target") == 0;
  free(out1);

  // we should get an absolute path for something with a smaller common prefix
  char *out2 = NULL;
  int r2 = disppath(path, "/", &out2);
  bool claim2_a = r2 == 0;
  bool claim2_b = claim2_a && strcmp(out2, "/") == 0;
  free(out2);

  // and we should get an absolute path for something with a different root
  char *out3 = NULL;
  int r3 = disppath(path, "/usr", &out3);
  bool claim3_a = access("/usr", R_OK) != 0 || r3 == 0;
  bool claim3_b =
      access("/usr", R_OK) != 0 || (r3 == 0 && strcmp(out3, "/usr") == 0);
  if (r3 == 0)
    free(out3);

  // clean up temporary file
  (void)unlink(target);

  // assert all our claims
  ASSERT(claim1_a);
  ASSERT(claim1_b);
  ASSERT(claim2_a);
  ASSERT(claim2_b);
  ASSERT(claim3_a);
  ASSERT(claim3_b);
}
