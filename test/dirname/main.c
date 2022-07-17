#include "../../clink/src/path.h"
#include "../test.h"
#include <stdlib.h>

TEST("test cases for clink/src/dirname.c:dirname()") {

  // dirname with invalid parameters should fail
  {
    const char in[] = "/hello/world";
    char *out = NULL;
    ASSERT_NE(dirname(NULL, &out), 0);
    ASSERT_NE(dirname(in, NULL), 0);
  }

  // some simple dirname cases
  {
    const char in[] = "/hello/world";
    char *out = NULL;
    ASSERT_EQ(dirname(in, &out), 0);
    ASSERT_STREQ(out, "/hello");
    free(out);
  }
  {
    const char in[] = "hello/world";
    char *out = NULL;
    ASSERT_EQ(dirname(in, &out), 0);
    ASSERT_STREQ(out, "hello");
    free(out);
  }
  {
    const char in[] = "/hello/world/foo/bar";
    char *out = NULL;
    ASSERT_EQ(dirname(in, &out), 0);
    ASSERT_STREQ(out, "/hello/world/foo");
    free(out);
  }
  {
    const char in[] = "hello/world/foo/bar";
    char *out = NULL;
    ASSERT_EQ(dirname(in, &out), 0);
    ASSERT_STREQ(out, "hello/world/foo");
    free(out);
  }

  // trailing /s should be ignored
  {
    const char in[] = "/hello/world/";
    char *out = NULL;
    ASSERT_EQ(dirname(in, &out), 0);
    ASSERT_STREQ(out, "/hello");
    free(out);
  }
  {
    const char in[] = "/hello/world//";
    char *out = NULL;
    ASSERT_EQ(dirname(in, &out), 0);
    ASSERT_STREQ(out, "/hello");
    free(out);
  }
  {
    const char in[] = "hello/world/";
    char *out = NULL;
    ASSERT_EQ(dirname(in, &out), 0);
    ASSERT_STREQ(out, "hello");
    free(out);
  }
  {
    const char in[] = "/hello/world/foo/bar/";
    char *out = NULL;
    ASSERT_EQ(dirname(in, &out), 0);
    ASSERT_STREQ(out, "/hello/world/foo");
    free(out);
  }
  {
    const char in[] = "hello/world/foo/bar/";
    char *out = NULL;
    ASSERT_EQ(dirname(in, &out), 0);
    ASSERT_STREQ(out, "hello/world/foo");
    free(out);
  }

  // cases that should evaluate to the root
  {
    const char in[] = "/hello";
    char *out = NULL;
    ASSERT_EQ(dirname(in, &out), 0);
    ASSERT_STREQ(out, "/");
    free(out);
  }
  {
    const char in[] = "/hello/";
    char *out = NULL;
    ASSERT_EQ(dirname(in, &out), 0);
    ASSERT_STREQ(out, "/");
    free(out);
  }
  {
    const char in[] = "/";
    char *out = NULL;
    ASSERT_EQ(dirname(in, &out), 0);
    ASSERT_STREQ(out, "/");
    free(out);
  }
  {
    const char in[] = "//";
    char *out = NULL;
    ASSERT_EQ(dirname(in, &out), 0);
    ASSERT_STREQ(out, "/");
    free(out);
  }
  {
    const char in[] = "///";
    char *out = NULL;
    ASSERT_EQ(dirname(in, &out), 0);
    ASSERT_STREQ(out, "/");
    free(out);
  }
}
