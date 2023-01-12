#include "debug.h"
#include "re.h"
#include <assert.h>
#include <errno.h>
#include <regex.h>
#include <stdlib.h>
#include <string.h>

int re_add(re_t **re, const char *regex) {
  assert(re != NULL);
  assert(regex != NULL);

  int rc = 0;
  re_t *r = NULL;

  r = calloc(1, sizeof(*r));
  if (ERROR(r == NULL)) {
    rc = ENOMEM;
    goto done;
  }

  r->expression = strdup(regex);
  if (ERROR(r->expression == NULL)) {
    rc = ENOMEM;
    goto done;
  }

  {
    int err = regcomp(&r->compiled, regex, REG_EXTENDED | REG_NOSUB);
    if (ERROR(err != 0)) {
      rc = re_err_to_errno(err);
      goto done;
    }
  }

  {
    re_t *head = __atomic_load_n(re, __ATOMIC_ACQUIRE);
    do {
      r->next = head;
    } while (!__atomic_compare_exchange_n(re, &head, r, true, __ATOMIC_RELEASE,
                                          __ATOMIC_ACQUIRE));
  }

done:
  if (rc != 0) {
    if (r != NULL)
      free(r->expression);
    free(r);
  }

  return rc;
}
