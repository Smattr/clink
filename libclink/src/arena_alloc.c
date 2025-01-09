#include "arena.h"
#include "debug.h"
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

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

/// an arbitrary `struct chunk` on which to call `sizeof` on its fields
///
/// We want to use `sizeof(some_chunk.content)` below, but all chunks are
/// addressed through `chunk_t *`s not `struct chunk *`s so this does not work.
/// This object gives us a way to do it that does not cause UB like the usual
/// tricks (`sizeof(((struct chunk *)0)->content)`).
static const struct chunk witness;

/// prepend a new chunk to allocate from
///
/// \param me Arena to expand
/// \return 0 on success or an errno on failure
static int add_chunk(arena_t *me) {
  assert(me != NULL);

  // create a new chunk
  chunk_t *const next = calloc(1, sizeof(struct chunk));
  if (ERROR(next == NULL))
    return ENOMEM;

    // posion the new, unallocated memory
#if 0
  POISON(next->content, sizeof(next->content));
#else
  POISON(next + offsetof(struct chunk, content), sizeof(witness.content));
#endif

  // make it the start of the arena’s linked-list
#if 0
  next->previous = me->current;
#else
  memcpy(next + offsetof(struct chunk, previous), &me->current,
         sizeof(me->current));
#endif
#if 0
  *me = (arena_t){.remaining = sizeof(next->content), .current = next};
#else
  *me = (arena_t){.remaining = sizeof(witness.content), .current = next};
#endif

  return 0;
}

void *arena_alloc(arena_t *me, size_t size) {
  assert(me != NULL);

  // we cannot allocate something that will not fit in a chunk
  if (ERROR(size > sizeof(witness.content)))
    return NULL;

  // if we do not have enough space, acquire a new chunk
  if (me->remaining < size) {
    if (ERROR(add_chunk(me) != 0))
      return NULL;
  }
  assert(me->remaining >= size);

  // allocate from the end of the chunk for simplicity
#if 0
  void *const answer = &me->current->content[me->remaining - size];
#else
  void *const answer =
      me->current + offsetof(struct chunk, content) + me->remaining - size;
#endif
  me->remaining -= size;

  // unpoison what we just allocated
  UNPOISON(answer, size);

  return answer;
}
