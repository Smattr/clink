#pragma once

#include <cstddef>
#include <cassert>

#ifdef NDEBUG
  #define unreachable() __builtin_unreachable()
#else
  #define unreachable() assert(!"unreachable")
#endif

