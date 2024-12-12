#include "fdbuf.h"
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>

void fdbuf_free(fdbuf_t *buffer) {
  assert(buffer != NULL);

  // restore the original description
  if (buffer->origin > 0) {
    (void)dup2(buffer->origin, buffer->target);
    (void)close(buffer->origin);
  }

  if (buffer->path != NULL)
    (void)unlink(buffer->path);
  free(buffer->path);

  *buffer = (fdbuf_t){0};
}
