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

/// size of a `chunk_t`
///
/// This is an internal implementation detail that should only be used by
/// arena*.[ch].
///
/// This is an arbitrary, hopefully-page-size-multiple value. It is chosen to
/// hopefully be large enough to span any single allocation a caller might need
/// to make.
enum { CHUNK_SIZE = 16384 };

/// an allocation block
///
/// This is an internal implementation detail that should only be used by
/// arena*.[ch].
///
/// The arena’s free list is stored as a linked-list of chunks from which to
/// allocate memory. The amount of free space remaining in a chunk is stored at
/// a higher level in the `arena_t` struct.
struct chunk {
  struct chunk *previous; ///< prior chunk in use
  char content[CHUNK_SIZE -
               sizeof(struct chunk *)]; ///< backing memory to be allocated
};

/// pointee type used for `struct chunk` pointers
///
/// The allocator itself wants to deal in `struct chunk`s but then hand out
/// pointers to arbitrary types. While strict aliasing allows `char *` to alias
/// other pointers it does not allow other pointers to alias `char[]`. So we
/// cannot address allocatable memory as the `content` member of `struct chunk`
/// but instead only refer to it through `chunk_t *`s.
typedef char chunk_t;

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
/// This function may fail for reasons other than out-of-memory, e.g. requested
/// size cannot be handled by this allocator. If you pass a 0 size, there is no
/// way to distinguish the success and failure cases.
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
