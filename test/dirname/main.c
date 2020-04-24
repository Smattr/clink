// test cases for ../../clink/src/dirname.c:dirname()

// force assertions on
#ifdef NDEBUG
  #undef NDEBUG
#endif

#include <assert.h>
#include "../../clink/src/path.h"
#include <stdlib.h>
#include <string.h>

int main(void) {

  // dirname with invalid parameters should fail
  {
    const char in[] = "/hello/world";
    char *out = NULL;
    assert(dirname(NULL, &out) != 0);
    assert(dirname(in, NULL) != 0);
  }

  // some simple dirname cases
  {
    const char in[] = "/hello/world";
    char *out = NULL;
    assert(dirname(in, &out) == 0);
    assert(strcmp(out, "/hello") == 0);
    free(out);
  }
  {
    const char in[] = "hello/world";
    char *out = NULL;
    assert(dirname(in, &out) == 0);
    assert(strcmp(out, "hello") == 0);
    free(out);
  }
  {
    const char in[] = "/hello/world/foo/bar";
    char *out = NULL;
    assert(dirname(in, &out) == 0);
    assert(strcmp(out, "/hello/world/foo") == 0);
    free(out);
  }
  {
    const char in[] = "hello/world/foo/bar";
    char *out = NULL;
    assert(dirname(in, &out) == 0);
    assert(strcmp(out, "hello/world/foo") == 0);
    free(out);
  }

  // trailing /s should be ignored
  {
    const char in[] = "/hello/world/";
    char *out = NULL;
    assert(dirname(in, &out) == 0);
    assert(strcmp(out, "/hello") == 0);
    free(out);
  }
  {
    const char in[] = "/hello/world//";
    char *out = NULL;
    assert(dirname(in, &out) == 0);
    assert(strcmp(out, "/hello") == 0);
    free(out);
  }
  {
    const char in[] = "hello/world/";
    char *out = NULL;
    assert(dirname(in, &out) == 0);
    assert(strcmp(out, "hello") == 0);
    free(out);
  }
  {
    const char in[] = "/hello/world/foo/bar/";
    char *out = NULL;
    assert(dirname(in, &out) == 0);
    assert(strcmp(out, "/hello/world/foo") == 0);
    free(out);
  }
  {
    const char in[] = "hello/world/foo/bar/";
    char *out = NULL;
    assert(dirname(in, &out) == 0);
    assert(strcmp(out, "hello/world/foo") == 0);
    free(out);
  }

  // cases that should evaluate to the root
  {
    const char in[] = "/hello";
    char *out = NULL;
    assert(dirname(in, &out) == 0);
    assert(strcmp(out, "/") == 0);
    free(out);
  }
  {
    const char in[] = "/hello/";
    char *out = NULL;
    assert(dirname(in, &out) == 0);
    assert(strcmp(out, "/") == 0);
    free(out);
  }
  {
    const char in[] = "/";
    char *out = NULL;
    assert(dirname(in, &out) == 0);
    assert(strcmp(out, "/") == 0);
    free(out);
  }
  {
    const char in[] = "//";
    char *out = NULL;
    assert(dirname(in, &out) == 0);
    assert(strcmp(out, "/") == 0);
    free(out);
  }
  {
    const char in[] = "///";
    char *out = NULL;
    assert(dirname(in, &out) == 0);
    assert(strcmp(out, "/") == 0);
    free(out);
  }

  return EXIT_SUCCESS;
}
