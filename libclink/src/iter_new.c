#include <clink/iter.h>
#include <errno.h>
#include "iter.h"
#include <stdlib.h>

int iter_new(clink_iter_t **it, no_lookahead_iter_t *impl) {

  if (it == NULL)
    return EINVAL;

  if (impl == NULL)
    return EINVAL;

  clink_iter_t *i = calloc(1, sizeof(*i));
  if (i == NULL)
    return ENOMEM;

  if (impl->next_str != NULL) {
    int rc = iter_str_new(i, impl);
    if (rc) {
      free(i);
    } else {
      *it = i;
    }
    return rc;
  }

  if (impl->next_symbol != NULL) {
    int rc = iter_symbol_new(i, impl);
    if (rc) {
      free(i);
    } else {
      *it = i;
    }
    return rc;
  }

  return ENOTSUP;
}
