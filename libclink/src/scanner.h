#pragma once

#include <stddef.h>

/// state for a generialised lexical scanner
typedef struct {
  const char *base;     ///< start of the input
  size_t size;          ///< number of bytes in the input
  size_t offset;        ///< current position in the input
  unsigned long lineno; ///< current position’s line number
  unsigned long colno;  ///< current position’s column number
} scanner_t;
