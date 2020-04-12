#include <clink/iter.h>
#include <clink/symbol.h>
#include <errno.h>
#include "iter.h"
#include <stdbool.h>
#include <stdlib.h>

bool clink_iter_has_next(const clink_iter_t *it) {

  if (it == NULL)
    return false;

  if (it->has_next == NULL)
    return false;

  return it->has_next(it);
}

int clink_iter_next_str(clink_iter_t *it, const char **yielded) {

  if (it == NULL)
    return EINVAL;

  if (yielded == NULL)
    return EINVAL;

  if (it->next_str == NULL)
    return EINVAL;

  return it->next_str(it, yielded);
}

int clink_iter_next_symbol(clink_iter_t *it, const clink_symbol_t **yielded) {

  if (it == NULL)
    return EINVAL;

  if (yielded == NULL)
    return EINVAL;

  if (it->next_symbol == NULL)
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
