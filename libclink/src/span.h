#pragma once

#include <assert.h>
#include <clink/symbol.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

/// a non-owning string reference
typedef struct {
  const char *base;       ///< start of the range
  size_t size;            ///< number of bytes in the range
  unsigned long lineno;   ///< originating source line number
  unsigned long colno;    ///< originating source column number
  clink_location_t start; ///< originating source start of semantic entity
  clink_location_t end;   ///< originating source end of semantic entity
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
