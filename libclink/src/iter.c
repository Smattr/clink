#include "iter.h"
#include "debug.h"
#include <clink/iter.h>
#include <clink/symbol.h>
#include <errno.h>
#include <stdlib.h>

int clink_iter_next_symbol(clink_iter_t *it, const clink_symbol_t **yielded) {

  if (ERROR(it == NULL))
    return EINVAL;

  if (ERROR(yielded == NULL))
    return EINVAL;

  if (ERROR(it->next_symbol == NULL))
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
