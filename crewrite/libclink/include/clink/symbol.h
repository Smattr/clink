#pragma once

#include <stddef.h>

typedef enum {
  CLINK_ST_DEFINITION,
  CLINK_ST_CALL,
  CLINK_ST_REFERENCE,
  CLINK_ST_INCLUDE,

  ST_RESERVED,
} clink_category_t;

typedef struct {
  clink_category_t category;
  const char *name;
  size_t name_len;
  const char *path;
  unsigned long line;
  unsigned long column;
  const char *parent;
} clink_symbol_t;
