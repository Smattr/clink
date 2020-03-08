#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

enum clink_category {
  CLINK_DEFINITION = 0,
  CLINK_FUNCTION_CALL = 1,
  CLINK_REFERENCE = 2,
  CLINK_INCLUDE = 3,
};

struct clink_symbol {
  enum clink_category category;
  char *name;
  size_t name_len;
  char *path;
  unsigned long lineno;
  unsigned long colno;
  char *parent;
};

struct clink_result {
  struct clink_symbol symbol;
  char *context;
};

#ifdef __cplusplus
}
#endif
