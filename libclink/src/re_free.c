#include "re.h"
#include <assert.h>
#include <regex.h>
#include <stdlib.h>

void re_free(void *re) {
  assert(re != NULL);

  re_t **rep = re;

  // synchronise with writes in `re_add`
  __atomic_thread_fence(__ATOMIC_ACQUIRE);

  for (re_t *r = *rep; r != NULL; r = *rep) {
    free(r->expression);
    regfree(&r->compiled);
    *rep = r->next;
    free(r);
  }
}
