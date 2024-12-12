#include "fdbuf.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void fdbuf_free(fdbuf_t *buffer) {
  assert(buffer != NULL);

  // ensure everything that has previously been written is flushed before doing
  // our replacement
  if (buffer->target != NULL)
    (void)fflush(buffer->target);

  // restore the original description
  if (buffer->origin != NULL) {
    (void)dup2(fileno(buffer->origin), fileno(buffer->target));
    (void)fclose(buffer->origin);
  }

  if (buffer->path != NULL)
    (void)unlink(buffer->path);
  free(buffer->path);

  *buffer = (fdbuf_t){0};
}
