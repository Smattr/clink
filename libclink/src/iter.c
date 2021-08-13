#include <clink/iter.h>
#include <clink/symbol.h>
#include "../../common/compiler.h"
#include <errno.h>
#include "iter.h"
#include <stdlib.h>

int clink_iter_next_str(clink_iter_t *it, const char **yielded) {

  if (UNLIKELY(it == NULL))
    return EINVAL;

  if (UNLIKELY(yielded == NULL))
    return EINVAL;

  if (UNLIKELY(it->next_str == NULL))
    return EINVAL;

  return it->next_str(it, yielded);
}

int clink_iter_next_symbol(clink_iter_t *it, const clink_symbol_t **yielded) {

  if (UNLIKELY(it == NULL))
    return EINVAL;

  if (UNLIKELY(yielded == NULL))
    return EINVAL;

  if (UNLIKELY(it->next_symbol == NULL))
    return EINVAL;

  return it->next_symbol(it, yielded);
}

void clink_iter_free(clink_iter_t **it) {

  // allow harmless freeing of NULL
  if (it == NULL || *it == NULL)
    return;

  // call this iterator’s specialised clean up function if it exists
  if ((*it)->free != NULL)
    (*it)->free(*it);

  free(*it);
  *it = NULL;
}
