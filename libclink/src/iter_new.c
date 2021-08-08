#include <clink/iter.h>
#include "../../common/compiler.h"
#include <errno.h>
#include "iter.h"
#include <stdlib.h>

int iter_new(clink_iter_t **it, no_lookahead_iter_t *impl) {

  if (UNLIKELY(it == NULL))
    return EINVAL;

  if (UNLIKELY(impl == NULL))
    return EINVAL;

  clink_iter_t *i = calloc(1, sizeof(*i));
  if (UNLIKELY(i == NULL))
    return ENOMEM;

  if (impl->next_str != NULL) {
    int rc = iter_str_new(i, impl);
    if (UNLIKELY(rc)) {
      free(i);
    } else {
      *it = i;
    }
    return rc;
  }

  if (impl->next_symbol != NULL) {
    int rc = iter_symbol_new(i, impl);
    if (UNLIKELY(rc)) {
      free(i);
    } else {
      *it = i;
    }
    return rc;
  }

  return ENOTSUP;
}
