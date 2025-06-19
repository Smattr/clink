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

/// default size of a `chunk_t`
///
/// This is an arbitrary, hopefully-page-size-multiple value.
enum { CHUNK_SIZE = 16384 };

/// find the beginning of a chunk’s allocation region
static char *pool(chunk_t *base) {
  assert(base != NULL);
  return (char *)base + sizeof(*base);
}

/// prepend a new chunk to allocate from
///
/// \param me Arena to expand
/// \return 0 on success or an errno on failure
static int add_chunk(arena_t *me, size_t request) {
  assert(me != NULL);

  // how much space should we allocate?
  size_t chunk_size = CHUNK_SIZE;
  if (chunk_size - sizeof(chunk_t) < request) {
    // The default allocation still will not be large enough to satisfy the
    // request. So enlarge this chunk.
    chunk_size = request + sizeof(chunk_t);
  }
  const size_t pool_size = chunk_size - sizeof(chunk_t);

  // Create a new chunk. We deliberately use something different from
  // `sizeof(*next)`. See the definition of `chunk_t` for an explanation of why.
  chunk_t *const next = calloc(1, chunk_size);
  if (ERROR(next == NULL))
    return ENOMEM;

  // poison the new, unallocated memory
  POISON(pool(next), pool_size);

  // make it the start of the arena’s linked-list
  next->previous = me->current;
  *me = (arena_t){.remaining = pool_size, .current = next};

  return 0;
}

void *arena_alloc(arena_t *me, size_t size) {
  assert(me != NULL);

  // simplify later arithmetic edge cases
  if (size == 0)
    return NULL;

  // if we do not have enough space, acquire a new chunk
  if (me->remaining < size) {
    if (ERROR(add_chunk(me, size) != 0))
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
