/// \file
/// \brief ctype.h replacements that assume “C” locale

#pragma once

#include <stdbool.h>
#include <stdint.h>

static inline bool isalpha_(intmax_t c) {
  if (c >= 'a' && c <= 'z')
    return true;
  if (c >= 'A' && c <= 'Z')
    return true;
  return false;
}

static inline bool iscntrl_(intmax_t c) {
  if (c >= 0 && c <= 31)
    return true;
  if (c == 127)
    return true;
  return false;
}

static inline bool isdigit_(intmax_t c) { return c >= '0' && c <= '9'; }

static inline bool isalnum_(intmax_t c) { return isalpha_(c) || isdigit_(c); }

static inline bool isspace_(intmax_t c) {
  if (c == ' ')
    return true;
  if (c == '\f')
    return true;
  if (c == '\n')
    return true;
  if (c == '\r')
    return true;
  if (c == '\t')
    return true;
  if (c == '\v')
    return true;
  return false;
}
