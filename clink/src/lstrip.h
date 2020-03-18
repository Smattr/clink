#pragma once

#include <ctype.h>

static inline const char *lstrip(const char *s) {

  const char *p = s;

  while (isspace(*p))
    p++;

  if (*p == '\0')
    return "\n";

  return p;
}
