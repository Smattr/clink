#include "../clink/src/path.h"
#include "test.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

TEST("test cases for clink/src/disppath.c:disppath()") {

  static const char target[] = "target";

  // disppath with invalid parameters should fail
  {
    char *out = NULL;
    ASSERT_NE(disppath(NULL, &out), 0);
    ASSERT_NE(disppath(target, NULL), 0);
  }

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

  // disppath on a local path should give us the relative path
  char *out1 = NULL;
  int r1 = disppath(target, &out1);
  bool claim1_a = r1 == 0;
  bool claim1_b = strcmp(out1, "target") == 0;
  free(out1);

  // the same if we provide a absolute path
  char *in2 = NULL;
  {
    int r = asprintf(&in2, "%s/%s", path, target);
    ASSERT_GE(r, 0);
  }
  char *out2 = NULL;
  int r2 = disppath(in2, &out2);
  bool claim2_a = r2 == 0;
  bool claim2_b = strcmp(out2, "target") == 0;
  free(out2);
  free(in2);

  // we should get an absolute path for something with a smaller common prefix
  char *out3 = NULL;
  int r3 = disppath("/", &out3);
  bool claim3_a = r3 == 0;
  bool claim3_b = strcmp(out3, "/") == 0;
  free(out3);

  // and we should get an absolute path for something with a different root
  char *out4 = NULL;
  int r4 = disppath("/usr", &out4);
  bool claim4_a = access("/usr", R_OK) != 0 || r4 == 0;
  bool claim4_b = access("/usr", R_OK) != 0 || strcmp(out4, "/usr") == 0;
  if (r4 == 0)
    free(out4);

  // clean up temporary file
  (void)unlink(target);

  // move out of this directory so that we can then remove it
  {
    int r = chdir("/");
    ASSERT_EQ(r, 0);
  }
  (void)rmdir(path);
  free(path);

  // assert all our claims
  ASSERT(claim1_a);
  ASSERT(claim1_b);
  ASSERT(claim2_a);
  ASSERT(claim2_b);
  ASSERT(claim3_a);
  ASSERT(claim3_b);
  ASSERT(claim4_a);
  ASSERT(claim4_b);
}