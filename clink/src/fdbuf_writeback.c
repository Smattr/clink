#include "fdbuf.h"
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

int fdbuf_writeback(const char *header, fdbuf_t *buffer, const char *footer) {
  assert(buffer != NULL);
  assert(buffer->origin != NULL && "writing back an uninitialised fd buffer");

  int rc = 0;

  // ensure any pending data is flushed through to our pipe before trying to
  // examine it
  if (fflush(buffer->target) < 0) {
    rc = errno;
    goto done;
  }

  // if our buffer is empty, nothing to be done
  if (ftell(buffer->target) == 0)
    return 0;

  // write the header
  if (header != NULL) {
    if (fputs(header, buffer->origin) < 0) {
      rc = EIO;
      goto done;
    }
  }

  // rewind to the start of the buffered data
  rewind(buffer->target);

  // copy data back to the original stream
  while (true) {
    char window[BUFSIZ] = {0};
    const size_t r = fread(window, 1, sizeof(window), buffer->target);
    assert(r <= sizeof(window));
    if (r == 0) {
      if (feof(buffer->target))
        break;
      rc = EIO;
      goto done;
    }
    if (fwrite(window, r, 1, buffer->origin) != 1) {
      rc = EIO;
      goto done;
    }
  }

  // write the footer
  if (footer != NULL) {
    if (fputs(footer, buffer->origin) < 0) {
      rc = EIO;
      goto done;
    }
  }

  // rewind back to the start of the buffer so it appears empty
  rewind(buffer->target);

done:
  return rc;
}
