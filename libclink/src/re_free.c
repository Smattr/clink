#include "re.h"
#include <assert.h>
#include <regex.h>
#include <stdlib.h>

void re_free(void *re) {
  assert(re != NULL);

  re_t **rep = re;

  for (re_t *r = *rep; r != NULL; r = *rep) {
    free(r->expression);
    regfree(&r->compiled);
    *rep = r->next;
    free(r);
  }
}
