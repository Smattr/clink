#include "fdbuf.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void fdbuf_free(fdbuf_t *buffer) {
  assert(buffer != NULL);

  // ensure everything that has previously been written is flushed before doing
  // our replacement
  if (buffer->subject != NULL)
    (void)fflush(buffer->subject);

  // restore the original description
  if (buffer->origin != NULL) {
    assert(buffer->subject != NULL);
    (void)dup2(fileno(buffer->origin), fileno(buffer->subject));
    (void)fclose(buffer->origin);
  }

  *buffer = (fdbuf_t){0};
}
