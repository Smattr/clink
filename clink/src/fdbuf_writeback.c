#include "fdbuf.h"
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int fdbuf_writeback(const char *header, fdbuf_t *buffer, const char *footer) {
  assert(buffer != NULL);
  assert(buffer->origin > 0 && "writing back an uninitialised fd buffer");

  int rc = 0;

  // write the header
  for (size_t i = 0; header != NULL && i < strlen(header);) {
    const ssize_t w = write(buffer->origin, &header[i], strlen(&header[i]));
    if (w < 0) {
      if (errno != EINTR) {
        rc = errno;
        goto done;
      }
      continue;
    }
    assert((size_t)w <= strlen(&header[i]));
    i += (size_t)w;
  }

  // rewind to the start of the buffered data
  if (lseek(buffer->target, 0, SEEK_SET) < 0) {
    rc = errno;
    goto done;
  }

  // copy data back to the original stream
  while (true) {
    char window[BUFSIZ] = {0};
    const ssize_t r = read(buffer->target, window, sizeof(window));
    if (r < 0) {
      if (errno != EINTR) {
        rc = errno;
        goto done;
      }
      continue;
    }
    if (r == 0)
      break;
    assert((size_t)r <= sizeof(window));
    for (size_t offset = 0; offset < (size_t)r;) {
      const ssize_t w =
          write(buffer->origin, &window[offset], (size_t)r - offset);
      if (w < 0) {
        if (errno != EINTR) {
          rc = errno;
          goto done;
        }
        continue;
      }
      assert((size_t)w <= (size_t)r - offset);
      offset += (size_t)w;
    }
  }

  // write the footer
  for (size_t i = 0; footer != NULL && i < strlen(footer);) {
    const ssize_t w = write(buffer->origin, &footer[i], strlen(&footer[i]));
    if (w < 0) {
      if (errno != EINTR) {
        rc = errno;
        goto done;
      }
      continue;
    }
    assert((size_t)w <= strlen(&footer[i]));
    i += (size_t)w;
  }

  // rewind back to the start of the buffer so it appears empty
  if (lseek(buffer->target, 0, SEEK_SET) < 0) {
    rc = errno;
    goto done;
  }

done:
  return rc;
}
