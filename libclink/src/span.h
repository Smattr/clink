#pragma once

#include <stddef.h>

/// a non-owning string reference
typedef struct {
  const char *base; ///< start of the range
  size_t size;      ///< number of bytes in the range
} span_t;
