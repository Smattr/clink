/// various compiler hint macros

#pragma once

#include <assert.h>

#define UNREACHABLE()                                                          \
  do {                                                                         \
    assert(0 && "unreachable");                                                \
    __builtin_unreachable();                                                   \
  } while (0)
