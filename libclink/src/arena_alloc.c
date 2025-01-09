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

/// the number of bytes available for allocation out of a chunk
enum { POOL_SIZE = CHUNK_SIZE - sizeof(chunk_t) };

/// find the beginning of a chunk’s allocation region
static char *pool(chunk_t *base) {
  assert(base != NULL);
  return (char *)base + sizeof(*base);
}

/// prepend a new chunk to allocate from
///
/// \param me Arena to expand
/// \return 0 on success or an errno on failure
static int add_chunk(arena_t *me) {
  assert(me != NULL);

  // Create a new chunk. We deliberately use something different from
  // `sizeof(*next)`. See the definition of `chunk_t` for an explanation of why.
  chunk_t *const next = calloc(1, CHUNK_SIZE);
  if (ERROR(next == NULL))
    return ENOMEM;

  // posion the new, unallocated memory
  POISON(pool(next), POOL_SIZE);

  // make it the start of the arena’s linked-list
  next->previous = me->current;
  *me = (arena_t){.remaining = POOL_SIZE, .current = next};

  return 0;
}

void *arena_alloc(arena_t *me, size_t size) {
  assert(me != NULL);

  // we cannot allocate something that will not fit in a chunk
  if (ERROR(size > POOL_SIZE))
    return NULL;

  // if we do not have enough space, acquire a new chunk
  if (me->remaining < size) {
    if (ERROR(add_chunk(me) != 0))
      return NULL;
  }
  assert(me->remaining >= size);

  // allocate from the end of the chunk for simplicity
  void *const answer = pool(me->current) + me->remaining - size;
  me->remaining -= size;

  // unpoison what we just allocated
  UNPOISON(answer, size);

  return answer;
}
