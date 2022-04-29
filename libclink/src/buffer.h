/// \file
/// \brief abstraction for an in-memory buffer

#include "../../common/compiler.h"
#include <stddef.h>
#include <stdio.h>

/// an in-memory buffer
typedef struct {
  char *base;
  size_t size;
  FILE *f;
} buffer_t;

/** create a new in-memory buffer
 *
 * \param b Buffer to initialise
 * \returns 0 on success or an errno on failure
 */
INTERNAL int buffer_open(buffer_t *b);

/** synchronise backing memory
 *
 * This should be called on a buffer prior to accessing either its `base` or
 * `size` members. Any write to its `f` member invalidates prior read `base` or
 * `size` until the next `buffer_sync` call.
 *
 * \param b Buffer to synchronise
 */
INTERNAL void buffer_sync(buffer_t *b);

/** discard buffer data and reset it for new use
 *
 * \param b Buffer to clear
 */
INTERNAL void buffer_clear(buffer_t *b);

/** destroy an in-memory buffer
 *
 * \param b Buffer to close
 */
INTERNAL void buffer_close(buffer_t *b);
