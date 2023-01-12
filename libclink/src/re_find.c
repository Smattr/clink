#include "../../common/compiler.h"
#include "re.h"
#include <assert.h>
#include <regex.h>
#include <stddef.h>
#include <string.h>

regex_t re_find(const re_t **re, const char *regex) {
  assert(re != NULL);
  assert(regex != NULL);

  const re_t *r = __atomic_load_n(re, __ATOMIC_ACQUIRE);
  while (r != NULL) {
    if (strcmp(r->expression, regex) == 0)
      return r->compiled;
    r = r->next;
  }

  UNREACHABLE();
}
