/// \file
/// \brief Arena-style memory allocator
///
/// This API provides a very simple arena memory allocator:¹
///   • No `free` is provided. Callers are expected to allocate memory as needed
///     and then free the entire arena at once.
///   • No care is taken to provide aligned pointers. It is assumed that callers
///     are alignment agnostic or are performing their own rounding and
///     alignment externally.
///
/// ¹ https://en.wikipedia.org/wiki/Region-based_memory_management

#pragma once

#include "../../common/compiler.h"
#include <stddef.h>

/// an allocation block
///
/// This is an internal implementation detail that should only be used by
/// arena*.[ch].
///
/// The arena’s free list is stored as a linked-list of chunks from which to
/// allocate memory. The amount of free space remaining in a chunk is stored at
/// a higher level in the `arena_t` struct.
typedef struct chunk {
  struct chunk *previous; ///< prior chunk in use
#if 0
  /// Conceptually, this member follows. It is not defined explicitly because we
  /// would then be allocating out of it, handing out typed pointers. While
  /// strict aliasing permits `char *` to alias other types, it does not permit
  /// other types to alias a `char[]`.
  char content[]; ///< backing memory to be allocated
#endif
} chunk_t;

/// an arena’s state
///
/// Conceptually all fields of this struct are private, and should only be
/// accessed in non-trivial ways by arena*.[ch].
typedef struct {
  size_t remaining; ///< free bytes remaining in the current chunk
  chunk_t *current; ///< chunk in use
} arena_t;

/// allocate memory
///
/// This function may fail for reasons other than out-of-memory. If you pass a 0
/// size, there is no way to distinguish the success and failure cases.
///
/// \param me Arena from which to allocate
/// \param size Bytes to allocate
/// \return A pointer to allocated memory on success or `NULL` on failure
INTERNAL void *arena_alloc(arena_t *me, size_t size);

/// deallocate all previously allocated memory
///
/// After calling this function the arena may be reused.
///
/// \param me Arena to reset
INTERNAL void arena_reset(arena_t *me);
