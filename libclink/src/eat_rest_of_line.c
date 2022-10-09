#include "scanner.h"
#include <assert.h>

void eat_rest_of_line(scanner_t *s) {

  assert(s != NULL);

  while (s->offset < s->size) {
    char c = s->base[s->offset];
    eat_one(s);
    if (c == '\n' || c == '\r')
      break;
  }
}
