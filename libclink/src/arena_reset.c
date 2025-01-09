#include "arena.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

void arena_reset(arena_t *me) {
  assert(me != NULL);

  while (me->current != NULL) {
#if 0
    chunk_t *const previous = me->current->previous;
#else
    chunk_t *previous;
    memcpy(&previous, me->current + offsetof(struct chunk, previous),
           sizeof(previous));
#endif
    free(me->current);
    me->current = previous;
  }

  *me = (arena_t){0};
}
