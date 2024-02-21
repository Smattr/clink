#include "../../common/ctype.h"
#include "scanner.h"
#include <assert.h>
#include <stdbool.h>

bool eat_non_ws(scanner_t *s) {
  assert(s != NULL);

  if (s->offset >= s->size)
    return false;
  if (isspace_(s->base[s->offset]))
    return false;

  while (s->offset < s->size && !isspace_(s->base[s->offset]))
    eat_one(s);

  return true;
}
