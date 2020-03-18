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

int clink_symbol_copy(struct clink_symbol *dst, const struct clink_symbol *src);

void clink_symbol_clear(struct clink_symbol *s);

struct clink_result {
  struct clink_symbol symbol;
  char *context;
};

int clink_result_copy(struct clink_result *dst, const struct clink_result *src);

void clink_result_clear(struct clink_result *r);

#ifdef __cplusplus
}
#endif
