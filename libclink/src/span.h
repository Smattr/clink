#pragma once

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

/// a non-owning string reference
typedef struct {
  const char *base; ///< start of the range
  size_t size;      ///< number of bytes in the range
} span_t;

static inline bool span_eq(span_t a, const char *b) {

  assert(a.base != NULL);
  assert(b != NULL);

  if (a.size != strlen(b))
    return false;

  if (strncmp(a.base, b, a.size) != 0)
    return false;

  return true;
}
