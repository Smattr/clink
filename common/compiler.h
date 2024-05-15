/// various compiler hint macros

#pragma once

#include <assert.h>

#ifdef __GNUC__
#define INTERNAL __attribute__((visibility("internal")))
#else
#define INTERNAL /* nothing */
#endif

#ifdef __GNUC__
#define LIKELY(expr) __builtin_expect(!!(expr), 1)
#define UNLIKELY(expr) __builtin_expect((expr), 0)
#else
#define LIKELY(expr) (expr)
#define UNLIKELY(expr) (expr)
#endif

#ifdef __GNUC__
#define UNREACHABLE()                                                          \
  do {                                                                         \
    assert(0 && "unreachable");                                                \
    __builtin_unreachable();                                                   \
  } while (0)
#elif defined(_MSC_VER)
#define UNREACHABLE()                                                          \
  do {                                                                         \
    assert(0 && "unreachable");                                                \
    __assume(0);                                                               \
  } while (0)
#else
#define UNREACHABLE() /* nothing */
#endif

#ifdef __GNUC__
#define UNUSED __attribute__((unused))
#else
#define UNUSED /* nothing */
#endif
