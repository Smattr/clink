#include "../common/compiler.h"
#include "test.h"
#include <assert.h>
#include <clink/clink.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/// state shared between test cases, `accept`, and `error`
typedef struct {
  const char **expected; ///< `NULL` terminated list of entries expected
  size_t next;           ///< which element of `expected` to check against next

  size_t errored_on; ///< if we failed, what element of `expected` was it on?
  char *error_entry; ///< optionally the entry we received
  const char *error_message; ///< optionally the error message we received
} state_t;

/// a suitable `accept` callback
static int accept(const char *name, void *context) {
  assert(name != NULL);
  assert(context != NULL);

  state_t *st = context;

  if (st->expected[st->next] == NULL) {
    st->errored_on = st->next;
    st->error_entry = strdup(name);
    ASSERT_NOT_NULL(st->error_entry);
    st->error_message = "more entries than expected";
    return ERANGE;
  }

  if (strcmp(name, st->expected[st->next]) != 0) {
    st->errored_on = st->next;
    st->error_entry = strdup(name);
    ASSERT_NOT_NULL(st->error_entry);
    st->error_message = "mismatched entry";
    return ENOENT;
  }

  ++st->next;
  return 0;
}

/// a suitable `error` callback
static int error(unsigned long lineno UNUSED, unsigned long colno UNUSED,
                 const char *message, void *context) {
  assert(message != NULL);
  assert(context != NULL);

  state_t *st = context;

  st->errored_on = st->next;
  st->error_message = message;

  return 0;
}

TEST("clink_parse_namefile basic") {

  // a list of some path names
  char content[] = "foo bar\nbaz/qux";

  // turn this into a file handle
  FILE *namefile = fmemopen(content, strlen(content), "r");
  ASSERT_NOT_NULL(namefile);

  state_t st = {.expected = (const char *[]){"foo", "bar", "baz/qux", NULL}};

  {
    int rc = clink_parse_namefile(namefile, accept, error, &st);

    if (rc)
      fprintf(stderr, "errored on entry %zu, %s: %s\n", st.errored_on,
              st.error_entry == NULL ? "<unavailable>" : st.error_entry,
              st.error_message);
    ASSERT_EQ(rc, 0);
  }

  ASSERT_EQ(st.errored_on, 0ul);
  ASSERT_EQ((void *)st.error_entry, NULL);
  ASSERT_EQ((void *)st.error_message, NULL);

  ASSERT_EQ(st.next, 3ul);

  free(st.error_entry);
  (void)fclose(namefile);
}

/// multiple separators should be coalesced
TEST("clink_parse_namefile multiple separators") {

  char content[] = "foo \n\t bar";
  FILE *namefile = fmemopen(content, strlen(content), "r");
  ASSERT_NOT_NULL(namefile);

  state_t st = {.expected = (const char *[]){"foo", "bar", NULL}};

  {
    int rc = clink_parse_namefile(namefile, accept, error, &st);

    if (rc)
      fprintf(stderr, "errored on entry %zu, %s: %s\n", st.errored_on,
              st.error_entry == NULL ? "<unavailable>" : st.error_entry,
              st.error_message);
    ASSERT_EQ(rc, 0);
  }

  ASSERT_EQ(st.errored_on, 0ul);
  ASSERT_EQ((void *)st.error_entry, NULL);
  ASSERT_EQ((void *)st.error_message, NULL);

  ASSERT_EQ(st.next, 2ul);

  free(st.error_entry);
  (void)fclose(namefile);
}

/// a trailing separator should be ignored
TEST("clink_parse_namefile trailing separators") {

  char content[] = "foo bar\n";
  FILE *namefile = fmemopen(content, strlen(content), "r");
  ASSERT_NOT_NULL(namefile);

  state_t st = {.expected = (const char *[]){"foo", "bar", NULL}};

  {
    int rc = clink_parse_namefile(namefile, accept, error, &st);

    if (rc)
      fprintf(stderr, "errored on entry %zu, %s: %s\n", st.errored_on,
              st.error_entry == NULL ? "<unavailable>" : st.error_entry,
              st.error_message);
    ASSERT_EQ(rc, 0);
  }

  ASSERT_EQ(st.errored_on, 0ul);
  ASSERT_EQ((void *)st.error_entry, NULL);
  ASSERT_EQ((void *)st.error_message, NULL);

  ASSERT_EQ(st.next, 2ul);

  free(st.error_entry);
  (void)fclose(namefile);
}

TEST("clink_parse_namefile empty") {

  // cannot use `fmemopen` for this one because some implementations like
  // FreeBSD do not support `size == 0`
  FILE *namefile = fopen("/dev/null", "r");
  ASSERT_NOT_NULL(namefile);

  state_t st = {.expected = (const char *[]){NULL}};

  {
    int rc = clink_parse_namefile(namefile, accept, error, &st);

    if (rc)
      fprintf(stderr, "errored on entry %zu, %s: %s\n", st.errored_on,
              st.error_entry == NULL ? "<unavailable>" : st.error_entry,
              st.error_message);
    ASSERT_EQ(rc, 0);
  }

  ASSERT_EQ(st.errored_on, 0ul);
  ASSERT_EQ((void *)st.error_entry, NULL);
  ASSERT_EQ((void *)st.error_message, NULL);

  ASSERT_EQ(st.next, 0ul);

  free(st.error_entry);
  (void)fclose(namefile);
}

TEST("clink_parse_namefile quoted") {

  char content[] = "foo \"bar/baz\" qux";
  FILE *namefile = fmemopen(content, strlen(content), "r");
  ASSERT_NOT_NULL(namefile);

  state_t st = {.expected = (const char *[]){"foo", "bar/baz", "qux", NULL}};

  {
    int rc = clink_parse_namefile(namefile, accept, error, &st);

    if (rc)
      fprintf(stderr, "errored on entry %zu, %s: %s\n", st.errored_on,
              st.error_entry == NULL ? "<unavailable>" : st.error_entry,
              st.error_message);
    ASSERT_EQ(rc, 0);
  }

  ASSERT_EQ(st.errored_on, 0ul);
  ASSERT_EQ((void *)st.error_entry, NULL);
  ASSERT_EQ((void *)st.error_message, NULL);

  ASSERT_EQ(st.next, 3ul);

  free(st.error_entry);
  (void)fclose(namefile);
}

TEST("clink_parse_namefile escape sequences") {

  char content[] = "foo \"ba\\\"r/ba\\\\z\" qux";
  FILE *namefile = fmemopen(content, strlen(content), "r");
  ASSERT_NOT_NULL(namefile);

  state_t st = {.expected =
                    (const char *[]){"foo", "ba\"r/ba\\z", "qux", NULL}};

  {
    int rc = clink_parse_namefile(namefile, accept, error, &st);

    if (rc)
      fprintf(stderr, "errored on entry %zu, %s: %s\n", st.errored_on,
              st.error_entry == NULL ? "<unavailable>" : st.error_entry,
              st.error_message);
    ASSERT_EQ(rc, 0);
  }

  ASSERT_EQ(st.errored_on, 0ul);
  ASSERT_EQ((void *)st.error_entry, NULL);
  ASSERT_EQ((void *)st.error_message, NULL);

  ASSERT_EQ(st.next, 3ul);

  free(st.error_entry);
  (void)fclose(namefile);
}

/// mismatched quotes should be rejected
TEST("clink_parse_namefile mismatched quote") {

  char content[] = "foo \"bar/baz";
  FILE *namefile = fmemopen(content, strlen(content), "r");
  ASSERT_NOT_NULL(namefile);

  state_t st = {.expected = (const char *[]){"foo", NULL}};

  {
    int rc = clink_parse_namefile(namefile, accept, error, &st);
    ASSERT_NE(rc, 0);
  }

  ASSERT_EQ(st.errored_on, 1ul);
  ASSERT_NOT_NULL(st.error_message);

  free(st.error_entry);
  (void)fclose(namefile);
}

TEST("clink_parse_namefile trailing quote") {

  char content[] = "foo \"";
  FILE *namefile = fmemopen(content, strlen(content), "r");
  ASSERT_NOT_NULL(namefile);

  state_t st = {.expected = (const char *[]){"foo", NULL}};

  {
    int rc = clink_parse_namefile(namefile, accept, error, &st);
    ASSERT_NE(rc, 0);
  }

  ASSERT_EQ(st.errored_on, 1ul);
  ASSERT_NOT_NULL(st.error_message);

  free(st.error_entry);
  (void)fclose(namefile);
}

/// generously accept the empty string as a path
TEST("clink_parse_namefile empty string") {

  char content[] = "foo \"\" bar";
  FILE *namefile = fmemopen(content, strlen(content), "r");
  ASSERT_NOT_NULL(namefile);

  state_t st = {.expected = (const char *[]){"foo", "", "bar", NULL}};

  {
    int rc = clink_parse_namefile(namefile, accept, error, &st);

    if (rc)
      fprintf(stderr, "errored on entry %zu, %s: %s\n", st.errored_on,
              st.error_entry == NULL ? "<unavailable>" : st.error_entry,
              st.error_message);
    ASSERT_EQ(rc, 0);
  }

  ASSERT_EQ(st.errored_on, 0ul);
  ASSERT_EQ((void *)st.error_entry, NULL);
  ASSERT_EQ((void *)st.error_message, NULL);

  ASSERT_EQ(st.next, 3ul);

  free(st.error_entry);
  (void)fclose(namefile);
}

/// escapes other than \" and \\ should be rejected
TEST("clink_parse_namefile bad escape") {

  char content[] = "foo \"bar/\\nbaz\" qux";
  FILE *namefile = fmemopen(content, strlen(content), "r");
  ASSERT_NOT_NULL(namefile);

  state_t st = {.expected = (const char *[]){"foo", NULL}};

  {
    int rc = clink_parse_namefile(namefile, accept, error, &st);
    ASSERT_NE(rc, 0);
  }

  ASSERT_EQ(st.errored_on, 1ul);
  ASSERT_NOT_NULL(st.error_message);

  free(st.error_entry);
  (void)fclose(namefile);
}

/// unquoted path with a quote should be rejected
TEST("clink_parse_namefile bare quote") {

  char content[] = "foo bar/\"baz qux";
  FILE *namefile = fmemopen(content, strlen(content), "r");
  ASSERT_NOT_NULL(namefile);

  state_t st = {.expected = (const char *[]){"foo", NULL}};

  {
    int rc = clink_parse_namefile(namefile, accept, error, &st);
    ASSERT_NE(rc, 0);
  }

  ASSERT_EQ(st.errored_on, 1ul);
  ASSERT_NOT_NULL(st.error_message);

  free(st.error_entry);
  (void)fclose(namefile);
}
