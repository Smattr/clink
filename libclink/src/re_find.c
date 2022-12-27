#include "../../common/compiler.h"
#include "re.h"
#include <assert.h>
#include <regex.h>
#include <stdatomic.h>
#include <stddef.h>
#include <string.h>

regex_t re_find(const re_t *_Atomic *re, const char *regex) {
  assert(re != NULL);
  assert(regex != NULL);

  const re_t *r = atomic_load_explicit(re, memory_order_acquire);
  while (r != NULL) {
    if (strcmp(r->expression, regex) == 0)
      return r->compiled;
    r = r->next;
  }

  UNREACHABLE();
}
