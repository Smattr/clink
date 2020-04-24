// test cases for ../../clink/src/join.c:join()

// force assertions on
#ifdef NDEBUG
  #undef NDEBUG
#endif

#include <assert.h>
#include "../../clink/src/path.h"
#include <stdlib.h>
#include <string.h>

int main(void) {

  // join with invalid parameters should fail
  {
    const char branch[] = "hello";
    const char stem[] = "world";
    char *result = NULL;
    assert(join(NULL, stem, &result) != 0);
    assert(join(branch, NULL, &result) != 0);
    assert(join(branch, stem, NULL) != 0);
  }
  {
    char *result = NULL;
    assert(join("", "world", &result) != 0);
    assert(join("hello", "", &result) != 0);
  }

  // some simple join cases
  {
    const char branch[] = "hello";
    const char stem[] = "world";
    char *result = NULL;
    assert(join(branch, stem, &result) == 0);
    assert(strcmp(result, "hello/world") == 0);
    free(result);
  }
  {
    const char branch[] = "/hello";
    const char stem[] = "world";
    char *result = NULL;
    assert(join(branch, stem, &result) == 0);
    assert(strcmp(result, "/hello/world") == 0);
    free(result);
  }
  {
    const char branch[] = "/hello";
    const char stem[] = "world/foo/bar";
    char *result = NULL;
    assert(join(branch, stem, &result) == 0);
    assert(strcmp(result, "/hello/world/foo/bar") == 0);
    free(result);
  }
  {
    const char branch[] = "/hello";
    const char stem[] = "world/foo/bar";
    char *result = NULL;
    assert(join(branch, stem, &result) == 0);
    assert(strcmp(result, "/hello/world/foo/bar") == 0);
    free(result);
  }

  // cases with redundant /s that should be eliminated
  {
    const char branch[] = "hello/";
    const char stem[] = "world";
    char *result = NULL;
    assert(join(branch, stem, &result) == 0);
    assert(strcmp(result, "hello/world") == 0);
    free(result);
  }
  {
    const char branch[] = "hello//";
    const char stem[] = "world";
    char *result = NULL;
    assert(join(branch, stem, &result) == 0);
    assert(strcmp(result, "hello/world") == 0);
    free(result);
  }
  {
    const char branch[] = "hello";
    const char stem[] = "/world";
    char *result = NULL;
    assert(join(branch, stem, &result) == 0);
    assert(strcmp(result, "hello/world") == 0);
    free(result);
  }
  {
    const char branch[] = "hello/";
    const char stem[] = "/world";
    char *result = NULL;
    assert(join(branch, stem, &result) == 0);
    assert(strcmp(result, "hello/world") == 0);
    free(result);
  }
  {
    const char branch[] = "hello//";
    const char stem[] = "//world";
    char *result = NULL;
    assert(join(branch, stem, &result) == 0);
    assert(strcmp(result, "hello/world") == 0);
    free(result);
  }

  // some root directory cases
  {
    const char branch[] = "/";
    const char stem[] = "hello";
    char *result = NULL;
    assert(join(branch, stem, &result) == 0);
    assert(strcmp(result, "/hello") == 0);
    free(result);
  }
  {
    const char branch[] = "/";
    const char stem[] = "hello/world";
    char *result = NULL;
    assert(join(branch, stem, &result) == 0);
    assert(strcmp(result, "/hello/world") == 0);
    free(result);
  }
  {
    const char branch[] = "/";
    const char stem[] = "/hello";
    char *result = NULL;
    assert(join(branch, stem, &result) == 0);
    assert(strcmp(result, "/hello") == 0);
    free(result);
  }

  return EXIT_SUCCESS;
}
