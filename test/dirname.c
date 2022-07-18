#include "../clink/src/path.h"
#include "test.h"
#include <stdlib.h>

TEST("dirname() with invalid parameters should fail") {
  const char in[] = "/hello/world";
  char *out = NULL;
  ASSERT_NE(dirname(NULL, &out), 0);
  ASSERT_NE(dirname(in, NULL), 0);
}

TEST("simple dirname() case 1") {
  const char in[] = "/hello/world";
  char *out = NULL;
  ASSERT_EQ(dirname(in, &out), 0);
  ASSERT_STREQ(out, "/hello");
  free(out);
}
TEST("simple dirname() case 2") {
  const char in[] = "hello/world";
  char *out = NULL;
  ASSERT_EQ(dirname(in, &out), 0);
  ASSERT_STREQ(out, "hello");
  free(out);
}
TEST("simple dirname() case 3") {
  const char in[] = "/hello/world/foo/bar";
  char *out = NULL;
  ASSERT_EQ(dirname(in, &out), 0);
  ASSERT_STREQ(out, "/hello/world/foo");
  free(out);
}
TEST("simple dirname() case 4") {
  const char in[] = "hello/world/foo/bar";
  char *out = NULL;
  ASSERT_EQ(dirname(in, &out), 0);
  ASSERT_STREQ(out, "hello/world/foo");
  free(out);
}

TEST("dirname() should ignored trailing /s 1") {
  const char in[] = "/hello/world/";
  char *out = NULL;
  ASSERT_EQ(dirname(in, &out), 0);
  ASSERT_STREQ(out, "/hello");
  free(out);
}
TEST("dirname() should ignored trailing /s 2") {
  const char in[] = "/hello/world//";
  char *out = NULL;
  ASSERT_EQ(dirname(in, &out), 0);
  ASSERT_STREQ(out, "/hello");
  free(out);
}
TEST("dirname() should ignored trailing /s 3") {
  const char in[] = "hello/world/";
  char *out = NULL;
  ASSERT_EQ(dirname(in, &out), 0);
  ASSERT_STREQ(out, "hello");
  free(out);
}
TEST("dirname() should ignored trailing /s 4") {
  const char in[] = "/hello/world/foo/bar/";
  char *out = NULL;
  ASSERT_EQ(dirname(in, &out), 0);
  ASSERT_STREQ(out, "/hello/world/foo");
  free(out);
}
TEST("dirname() should ignored trailing /s 5") {
  const char in[] = "hello/world/foo/bar/";
  char *out = NULL;
  ASSERT_EQ(dirname(in, &out), 0);
  ASSERT_STREQ(out, "hello/world/foo");
  free(out);
}

TEST("dirname() should normalise root result 1") {
  const char in[] = "/hello";
  char *out = NULL;
  ASSERT_EQ(dirname(in, &out), 0);
  ASSERT_STREQ(out, "/");
  free(out);
}
TEST("dirname() should normalise root result 2") {
  const char in[] = "/hello/";
  char *out = NULL;
  ASSERT_EQ(dirname(in, &out), 0);
  ASSERT_STREQ(out, "/");
  free(out);
}
TEST("dirname() should normalise root result 3") {
  const char in[] = "/";
  char *out = NULL;
  ASSERT_EQ(dirname(in, &out), 0);
  ASSERT_STREQ(out, "/");
  free(out);
}
TEST("dirname() should normalise root result 4") {
  const char in[] = "//";
  char *out = NULL;
  ASSERT_EQ(dirname(in, &out), 0);
  ASSERT_STREQ(out, "/");
  free(out);
}
TEST("dirname() should normalise root result 5") {
  const char in[] = "///";
  char *out = NULL;
  ASSERT_EQ(dirname(in, &out), 0);
  ASSERT_STREQ(out, "/");
  free(out);
}
