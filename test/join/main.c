#include "../../clink/src/path.h"
#include "../test.h"
#include <stdlib.h>
#include <string.h>

TEST("test cases for clink/src/join.c:join()") {

  // join with invalid parameters should fail
  {
    const char branch[] = "hello";
    const char stem[] = "world";
    char *result = NULL;
    ASSERT_NE(join(NULL, stem, &result), 0);
    ASSERT_NE(join(branch, NULL, &result), 0);
    ASSERT_NE(join(branch, stem, NULL), 0);
  }
  {
    char *result = NULL;
    ASSERT_NE(join("", "world", &result), 0);
    ASSERT_NE(join("hello", "", &result), 0);
  }

  // some simple join cases
  {
    const char branch[] = "hello";
    const char stem[] = "world";
    char *result = NULL;
    ASSERT_EQ(join(branch, stem, &result), 0);
    ASSERT_STREQ(result, "hello/world");
    free(result);
  }
  {
    const char branch[] = "/hello";
    const char stem[] = "world";
    char *result = NULL;
    ASSERT_EQ(join(branch, stem, &result), 0);
    ASSERT_STREQ(result, "/hello/world");
    free(result);
  }
  {
    const char branch[] = "/hello";
    const char stem[] = "world/foo/bar";
    char *result = NULL;
    ASSERT_EQ(join(branch, stem, &result), 0);
    ASSERT_STREQ(result, "/hello/world/foo/bar");
    free(result);
  }
  {
    const char branch[] = "/hello";
    const char stem[] = "world/foo/bar";
    char *result = NULL;
    ASSERT_EQ(join(branch, stem, &result), 0);
    ASSERT_STREQ(result, "/hello/world/foo/bar");
    free(result);
  }

  // cases with redundant /s that should be eliminated
  {
    const char branch[] = "hello/";
    const char stem[] = "world";
    char *result = NULL;
    ASSERT_EQ(join(branch, stem, &result), 0);
    ASSERT_STREQ(result, "hello/world");
    free(result);
  }
  {
    const char branch[] = "hello//";
    const char stem[] = "world";
    char *result = NULL;
    ASSERT_EQ(join(branch, stem, &result), 0);
    ASSERT_STREQ(result, "hello/world");
    free(result);
  }
  {
    const char branch[] = "hello";
    const char stem[] = "/world";
    char *result = NULL;
    ASSERT_EQ(join(branch, stem, &result), 0);
    ASSERT_STREQ(result, "hello/world");
    free(result);
  }
  {
    const char branch[] = "hello/";
    const char stem[] = "/world";
    char *result = NULL;
    ASSERT_EQ(join(branch, stem, &result), 0);
    ASSERT_STREQ(result, "hello/world");
    free(result);
  }
  {
    const char branch[] = "hello//";
    const char stem[] = "//world";
    char *result = NULL;
    ASSERT_EQ(join(branch, stem, &result), 0);
    ASSERT_STREQ(result, "hello/world");
    free(result);
  }

  // some root directory cases
  {
    const char branch[] = "/";
    const char stem[] = "hello";
    char *result = NULL;
    ASSERT_EQ(join(branch, stem, &result), 0);
    ASSERT_STREQ(result, "/hello");
    free(result);
  }
  {
    const char branch[] = "/";
    const char stem[] = "hello/world";
    char *result = NULL;
    ASSERT_EQ(join(branch, stem, &result), 0);
    ASSERT_STREQ(result, "/hello/world");
    free(result);
  }
  {
    const char branch[] = "/";
    const char stem[] = "/hello";
    char *result = NULL;
    ASSERT_EQ(join(branch, stem, &result), 0);
    ASSERT_STREQ(result, "/hello");
    free(result);
  }
}
