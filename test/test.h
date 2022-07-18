#pragma once

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct _test_case {
  void (*function)(void);
  const char *description;
  struct _test_case *next;
} test_case_t;

extern test_case_t *test_cases;

#define _JOIN(x, y) x##y
#define JOIN(x, y) _JOIN(x, y)

extern bool has_assertion_;

#define TEST(desc)                                                             \
  static void JOIN(test_, __LINE__)(void);                                     \
  static void __attribute__((constructor)) JOIN(add_test_, __LINE__)(void) {   \
    static test_case_t JOIN(test_case_, __LINE__) = {                          \
        .function = JOIN(test_, __LINE__),                                     \
        .description = desc,                                                   \
    };                                                                         \
    for (test_case_t **t = &test_cases;; t = &(*t)->next) {                    \
      if (*t == NULL || strcmp((desc), (*t)->description) < 0) {               \
        JOIN(test_case_, __LINE__).next = *t;                                  \
        *t = &JOIN(test_case_, __LINE__);                                      \
        return;                                                                \
      }                                                                        \
      if (strcmp((desc), (*t)->description) == 0) {                            \
        fprintf(stderr, "duplicate test cases \"%s\"\n", (desc));              \
        abort();                                                               \
      }                                                                        \
    }                                                                          \
    __builtin_unreachable();                                                   \
  }                                                                            \
  static void JOIN(test_, __LINE__)(void)

#define PRINT_FMT(x)                                                           \
  _Generic((x),                                                                \
           int: "%d",                                                          \
           long: "%ld",                                                        \
           long long: "%lld", \
           unsigned long: "%lu",                                               \
           const void*: "%p")

#define ASSERT_(a, a_name, op, b, b_name)                                      \
  do {                                                                         \
    has_assertion_ = true;                                                     \
    __typeof__(a) _a = (a);                                                    \
    __typeof__(b) _b = (b);                                                    \
    if (!(_a op _b)) {                                                         \
      fprintf(stderr, "failed\n    %s:%d: assertion “%s %s %s” failed\n",      \
              __FILE__, __LINE__, #a_name, #op, #b_name);                      \
      fprintf(stderr, "      %s = ", #a_name);                                 \
      fprintf(stderr, PRINT_FMT(_a), _a);                                      \
      fprintf(stderr, "\n");                                                   \
      fprintf(stderr, "      %s = ", #b_name);                                 \
      fprintf(stderr, PRINT_FMT(_b), _b);                                      \
      fprintf(stderr, "\n");                                                   \
      fflush(stderr);                                                          \
      abort();                                                                 \
    }                                                                          \
  } while (0)

#define ASSERT_EQ(a, b) ASSERT_(a, a, ==, b, b)
#define ASSERT_GE(a, b) ASSERT_(a, b, >=, b, b)
#define ASSERT_GT(a, b) ASSERT_(a, a, >, b, b)
#define ASSERT_NE(a, b) ASSERT_(a, a, !=, b, b)

#define ASSERT(expr)                                                           \
  do {                                                                         \
    has_assertion_ = true;                                                     \
    if (!(expr)) {                                                             \
      fprintf(stderr, "failed\n    %s:%d: assertion “%s” failed\n", __FILE__,  \
              __LINE__, #expr);                                                \
      abort();                                                                 \
    }                                                                          \
  } while (0)

#define ASSERT_NOT_NULL(p)                                                     \
  do {                                                                         \
    const void *_p = (p);                                                      \
    ASSERT_(_p, p, !=, (const void *)NULL, NULL);                              \
  } while (0)

#define ASSERT_STREQ(a, b)                                                     \
  do {                                                                         \
    has_assertion_ = true;                                                     \
    const char *_a = (a);                                                      \
    const char *_b = (b);                                                      \
    if (strcmp(_a, _b) != 0) {                                                 \
      fprintf(stderr,                                                          \
              "failed\n    %s:%d: assertion “strcmp(%s, %s) == 0” failed\n",   \
              __FILE__, __LINE__, #a, #b);                                     \
      fprintf(stderr, "      %s = \"%s\"\n", #a, _a);                          \
      fprintf(stderr, "      %s = \"%s\"\n", #b, _b);                          \
      fflush(stderr);                                                          \
      abort();                                                                 \
    }                                                                          \
  } while (0)

#define ASSERT_STRNE(a, b)                                                     \
  do {                                                                         \
    has_assertion_ = true;                                                     \
    const char *_a = (a);                                                      \
    const char *_b = (b);                                                      \
    if (strcmp(_a, _b) == 0) {                                                 \
      fprintf(stderr,                                                          \
              "failed\n    %s:%d: assertion “strcmp(%s, %s) != 0” failed\n",   \
              __FILE__, __LINE__, #a, #b);                                     \
      fprintf(stderr, "      %s = \"%s\"\n", #a, _a);                          \
      fprintf(stderr, "      %s = \"%s\"\n", #b, _b);                          \
      fflush(stderr);                                                          \
      abort();                                                                 \
    }                                                                          \
  } while (0)

#define FAIL(args...)                                                          \
  do {                                                                         \
    fprintf(stderr, "failed\n    ");                                           \
    fprintf(stderr, args);                                                     \
    fflush(stderr);                                                            \
    abort();                                                                   \
  } while (0)
