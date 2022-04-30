/// various compiler hint macros

#pragma once

#include <assert.h>

#define INTERNAL __attribute__((visibility("internal")))

#define LIKELY(expr) __builtin_expect(!!(expr), 1)
#define UNLIKELY(expr) __builtin_expect((expr), 0)

#define UNREACHABLE()                                                          \
  do {                                                                         \
    assert(0 && "unreachable");                                                \
    __builtin_unreachable();                                                   \
  } while (0)
