#include "../clink/src/path.h"
#include "test.h"
#include <stdlib.h>
#include <string.h>

TEST("join() with invalid parameters should fail 1") {
  const char branch[] = "hello";
  const char stem[] = "world";
  char *result = NULL;
  ASSERT_NE(join(NULL, stem, &result), 0);
  ASSERT_NE(join(branch, NULL, &result), 0);
  ASSERT_NE(join(branch, stem, NULL), 0);
}
TEST("join() with invalid parameters should fail 2") {
  char *result = NULL;
  ASSERT_NE(join("", "world", &result), 0);
  ASSERT_NE(join("hello", "", &result), 0);
}

TEST("simple join() 1") {
  const char branch[] = "hello";
  const char stem[] = "world";
  char *result = NULL;
  ASSERT_EQ(join(branch, stem, &result), 0);
  ASSERT_STREQ(result, "hello/world");
  free(result);
}
TEST("simple join() 2") {
  const char branch[] = "/hello";
  const char stem[] = "world";
  char *result = NULL;
  ASSERT_EQ(join(branch, stem, &result), 0);
  ASSERT_STREQ(result, "/hello/world");
  free(result);
}
TEST("simple join() 3") {
  const char branch[] = "/hello";
  const char stem[] = "world/foo/bar";
  char *result = NULL;
  ASSERT_EQ(join(branch, stem, &result), 0);
  ASSERT_STREQ(result, "/hello/world/foo/bar");
  free(result);
}
TEST("simple join() 4") {
  const char branch[] = "/hello";
  const char stem[] = "world/foo/bar";
  char *result = NULL;
  ASSERT_EQ(join(branch, stem, &result), 0);
  ASSERT_STREQ(result, "/hello/world/foo/bar");
  free(result);
}

TEST("join() with redundant / should be eliminated 1") {
  const char branch[] = "hello/";
  const char stem[] = "world";
  char *result = NULL;
  ASSERT_EQ(join(branch, stem, &result), 0);
  ASSERT_STREQ(result, "hello/world");
  free(result);
}
TEST("join() with redundant / should be eliminated 2") {
  const char branch[] = "hello//";
  const char stem[] = "world";
  char *result = NULL;
  ASSERT_EQ(join(branch, stem, &result), 0);
  ASSERT_STREQ(result, "hello/world");
  free(result);
}
TEST("join() with redundant / should be eliminated 3") {
  const char branch[] = "hello";
  const char stem[] = "/world";
  char *result = NULL;
  ASSERT_EQ(join(branch, stem, &result), 0);
  ASSERT_STREQ(result, "hello/world");
  free(result);
}
TEST("join() with redundant / should be eliminated 4") {
  const char branch[] = "hello/";
  const char stem[] = "/world";
  char *result = NULL;
  ASSERT_EQ(join(branch, stem, &result), 0);
  ASSERT_STREQ(result, "hello/world");
  free(result);
}
TEST("join() with redundant / should be eliminated 5") {
  const char branch[] = "hello//";
  const char stem[] = "//world";
  char *result = NULL;
  ASSERT_EQ(join(branch, stem, &result), 0);
  ASSERT_STREQ(result, "hello/world");
  free(result);
}

TEST("join() with root directory 1") {
  const char branch[] = "/";
  const char stem[] = "hello";
  char *result = NULL;
  ASSERT_EQ(join(branch, stem, &result), 0);
  ASSERT_STREQ(result, "/hello");
  free(result);
}
TEST("join() with root directory 2") {
  const char branch[] = "/";
  const char stem[] = "hello/world";
  char *result = NULL;
  ASSERT_EQ(join(branch, stem, &result), 0);
  ASSERT_STREQ(result, "/hello/world");
  free(result);
}
TEST("join() with root directory 3") {
  const char branch[] = "/";
  const char stem[] = "/hello";
  char *result = NULL;
  ASSERT_EQ(join(branch, stem, &result), 0);
  ASSERT_STREQ(result, "/hello");
  free(result);
}
