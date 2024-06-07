#include "mmap.h"
#include "../../common/compiler.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

int mmap_open(mmap_t *m, const char *filename) {

  assert(m != NULL);
  assert(filename != NULL);

  *m = (mmap_t){0};

  int rc = 0;
  int fd = -1;

  fd = open(filename, O_RDONLY | O_CLOEXEC);
  if (fd < 0) {
    rc = errno;
    goto done;
  }

  struct stat st;
  if (UNLIKELY(fstat(fd, &st) < 0)) {
    rc = errno;
    goto done;
  }
  size_t size = (size_t)st.st_size;

  // if the file is 0-sized, we do not need to map it
  if (size == 0)
    goto done;

  void *base = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (base == MAP_FAILED) {
    rc = errno;
    goto done;
  }

  // success
  m->base = base;
  m->size = size;

done:
  if (fd >= 0)
    (void)close(fd);

  return rc;
}

void mmap_close(mmap_t m) {

  assert(m.base != MAP_FAILED);

  if (m.base == NULL)
    return;

  (void)munmap(m.base, m.size);
}
