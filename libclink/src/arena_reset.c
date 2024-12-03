#include "arena.h"
#include <assert.h>
#include <stdlib.h>

void arena_reset(arena_t *me) {
  assert(me != NULL);

  while (me->current != NULL) {
    chunk_t *const previous = me->current->previous;
    free(me->current);
    me->current = previous;
  }

  *me = (arena_t){0};
}
