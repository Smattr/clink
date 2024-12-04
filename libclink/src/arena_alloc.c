#include "arena.h"
#include "debug.h"
#include <assert.h>
#include <errno.h>
#include <stdlib.h>

#ifdef __has_feature
#if __has_feature(address_sanitizer)
#include <sanitizer/asan_interface.h>
#define POISON(addr, size) ASAN_POISON_MEMORY_REGION((addr), (size))
#define UNPOISON(addr, size) ASAN_UNPOISON_MEMORY_REGION((addr), (size))
#endif
#endif

#ifndef POISON
#define POISON(addr, size)                                                     \
  do {                                                                         \
  } while (0)
#endif
#ifndef UNPOISON
#define UNPOISON(addr, size)                                                   \
  do {                                                                         \
  } while (0)
#endif

/// prepend a new chunk to allocate from
///
/// \param me Arena to expand
/// \return 0 on success or an errno on failure
static int add_chunk(arena_t *me) {
  assert(me != NULL);

  // create a new chunk
  chunk_t *const next = calloc(1, sizeof(*next));
  if (ERROR(next == NULL))
    return ENOMEM;

  // posion the new, unallocated memory
  POISON(next->content, sizeof(next->content));

  // make it the start of the arena’s linked-list
  next->previous = me->current;
  *me = (arena_t){.remaining = sizeof(next->content), .current = next};

  return 0;
}

void *arena_alloc(arena_t *me, size_t size) {
  assert(me != NULL);

  // we cannot allocate something that will not fit in a chunk
  if (ERROR(size > sizeof(me->current->content)))
    return NULL;

  // if we do not have enough space, acquire a new chunk
  if (me->remaining < size) {
    if (ERROR(add_chunk(me) != 0))
      return NULL;
  }
  assert(me->remaining >= size);

  // allocate from the end of the chunk for simplicity
  void *const answer = &me->current->content[me->remaining - size];
  me->remaining -= size;

  // unpoison what we just allocated
  UNPOISON(answer, size);

  return answer;
}
