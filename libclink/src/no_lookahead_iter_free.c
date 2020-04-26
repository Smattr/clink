#include "iter.h"
#include <stdlib.h>

void no_lookahead_iter_free(no_lookahead_iter_t **it) {

  // allow harmless free of null
  if (it == NULL || *it == NULL)
    return;

  (*it)->free(*it);

  free(*it);
  *it = NULL;
}
