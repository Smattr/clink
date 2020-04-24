// test cases for ../../clink/src/disppath.c:disppath()

// force assertions on
#ifdef NDEBUG
  #undef NDEBUG
#endif

#include <assert.h>
#include "../../clink/src/path.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(void) {

  static const char target[] = "target";

  // disppath with invalid parameters should fail
  {
    char *out = NULL;
    assert(disppath(NULL, &out) != 0);
    assert(disppath(target, NULL) != 0);
  }

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

  // change to this directory
  {
    int r = chdir(path);
    assert(r == 0);
  }

  // create a temporary file within this temporary directory
  {
    FILE *f = fopen(target, "w");
    assert(f != NULL);
    fprintf(f, "hello world\n");
    (void)fclose(f);
  }

  // disppath on a local path should give us the relative path
  char *out1 = NULL;
  int r1 = disppath(target, &out1);
  bool claim1_a = r1 == 0;
  bool claim1_b = strcmp(out1, "target") == 0;

  // the same if we provide a absolute path
  char *in2 = NULL;
  {
    int r = asprintf(&in2, "%s/%s", path, target);
    assert(r >= 0);
  }
  char *out2 = NULL;
  int r2 = disppath(in2, &out2);
  bool claim2_a = r2 == 0;
  bool claim2_b = strcmp(out2, "target") == 0;

  // we should get an absolute path for something with a smaller common prefix
  char *out3 = NULL;
  int r3 = disppath("/", &out3);
  bool claim3_a = r3 == 0;
  bool claim3_b = strcmp(out3, "/") == 0;

  // and we should get an absolute path for something with a different root
  char *out4 = NULL;
  int r4 = disppath("/usr", &out4);
  bool claim4_a = access("/usr", R_OK) != 0 || r4 == 0;
  bool claim4_b = access("/usr", R_OK) != 0 || strcmp(out4, "/usr") == 0;

  // clean up temporary file
  (void)unlink(target);

  // move out of this directory so that we can then remove it
  {
    int r = chdir("/");
    assert(r == 0);
  }
  (void)rmdir(path);

  // assert all our claims
  assert(claim1_a);
  assert(claim1_b);
  assert(claim2_a);
  assert(claim2_b);
  assert(claim3_a);
  assert(claim3_b);
  assert(claim4_a);
  assert(claim4_b);

  return EXIT_SUCCESS;
}
