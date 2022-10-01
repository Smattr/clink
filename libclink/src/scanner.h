#pragma once

#include "../../common/compiler.h"
#include <stdbool.h>
#include <stddef.h>

/// state for a generalised lexical scanner
typedef struct {
  const char *base;     ///< start of the input
  size_t size;          ///< number of bytes in the input
  size_t offset;        ///< current position in the input
  unsigned long lineno; ///< current position’s line number
  unsigned long colno;  ///< current position’s column number
} scanner_t;

/// advance and return true if the next character(s) are an end of line
INTERNAL bool eat_eol(scanner_t *s);

/// advance and return true if the expected identifier is next
INTERNAL bool eat_id(scanner_t *s, const char *expected);

/// advance and return true if the expected text is next
INTERNAL bool eat_if(scanner_t *s, const char *expected);

/// advance over a non-negative integer
INTERNAL bool eat_num(scanner_t *s, size_t *number);

/// advance over non white space
INTERNAL bool eat_non_ws(scanner_t *s);

/// advance one character
INTERNAL void eat_one(scanner_t *s);

/// advance over white space
INTERNAL void eat_ws(scanner_t *s);

/// advance over white space until the end of the line
INTERNAL void eat_ws_to_eol(scanner_t *s);
