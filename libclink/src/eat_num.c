#include "scanner.h"
#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

bool eat_num(scanner_t *s, size_t *number) {
  assert(s != NULL);

  if (s->offset >= s->size)
    return false;
  if (!isdigit(s->base[s->offset]))
    return false;

  // TODO: update line/col informaton; eat_one?

  size_t n = 0;
  size_t i;
  for (i = s->offset; i < s->size && isdigit(s->base[i]); ++i) {
    if (SIZE_MAX / 10 < n) // overflow
      return false;
    n *= 10;
    size_t digit = (size_t)(s->base[i] - '0');
    if (SIZE_MAX - digit < n) // overflow
      return false;
    n += digit;
  }

  s->offset = i;

  if (number != NULL)
    *number = n;

  return true;
}
