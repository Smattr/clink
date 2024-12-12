#include "debug.h"
#include "fdbuf.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

/// create a temporary file
///
/// \param out [out] An R/W descriptor on success
/// \return 0 on success or an errno on failure
static int make_temp(int *out) {
  assert(out != NULL);

  char *path = NULL;
  int fd = -1;
  int rc = 0;

#ifdef __linux__
#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 17, 0)
#if defined(__GLIBC__)
#if __GLIBC__ > 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 27)
  fd = memfd_create("clink stderr shadow", MFD_CLOEXEC);
  if (fd >= 0) {
    // success
    *out = fd;
    fd = -1;
    goto done;
  }
  DEBUG("memfd_create was attempted but failed: %s", strerror(errno));

  // if we failed, fallback on regular temporary file creation
#endif
#endif
#endif
#endif

  // find temporary storage space
  const char *TMPDIR = getenv("TMPDIR");
  if (TMPDIR == NULL)
    TMPDIR = "/tmp";

  // create a temporary path
  if (ERROR(asprintf(&path, "%s/temp.XXXXXX", TMPDIR) < 0)) {
    rc = ENOMEM;
    goto done;
  }

  // create a file there
  fd = mkostemp(path, O_CLOEXEC);
  if (ERROR(fd < 0)) {
    rc = errno;
    goto done;
  }

  // we can remove it from disk now that we have an open descriptor to it
  if (ERROR(unlink(path) < 0)) {
    rc = errno;
    goto done;
  }

  // success
  *out = fd;
  fd = -1;

done:
  if (fd >= 0)
    (void)close(fd);
  free(path);

  return rc;
}

int fdbuf_new(fdbuf_t *buffer, FILE *subject) {
  assert(buffer != NULL);
  assert(subject != NULL);

  *buffer = (fdbuf_t){0};
  int copy = -1;
  int fd = -1;
  FILE *origin = NULL;
  int rc = 0;

  // it should be OK to use `ERROR` in the following code, even though the
  // stream we are replacing might be stderr. The actual `dup2` is the last
  // action we take, so any `ERROR` encountered should write to the actual
  // stderr.

  // ensure any pending data is flushed before we begin replacing the descriptor
  if (ERROR(fflush(subject) < 0)) {
    rc = errno;
    goto done;
  }

  // duplicate the original descriptor so we can later restore it
  const int subject_fd = fileno(subject);
  if (ERROR(subject_fd < 0)) {
    rc = errno;
    goto done;
  }
  copy = dup(subject_fd);
  if (ERROR(copy < 0)) {
    rc = errno;
    goto done;
  }
  // assume we are single threaded and thus this is not racy
  {
    const int flags = fcntl(copy, F_GETFD);
    if (ERROR(fcntl(copy, F_SETFD, flags | FD_CLOEXEC) < 0)) {
      rc = errno;
      goto done;
    }
  }

  // turn the copy into a file handle so we can more easily manage it
  origin = fdopen(copy, "w");
  if (ERROR(origin == NULL)) {
    rc = errno;
    goto done;
  }
  copy = -1;

  // create a temporary file
  if (ERROR((rc = make_temp(&fd))))
    goto done;

  // dup this over the subject
  if (ERROR(dup2(fd, subject_fd) < 0)) {
    rc = errno;
    goto done;
  }

  // success
  *buffer = (fdbuf_t){.subject = subject, .origin = origin};
  origin = NULL;

done:
  if (fd >= 0)
    (void)close(fd);
  if (origin != NULL)
    (void)fclose(origin);
  if (copy >= 0)
    (void)close(copy);

  return rc;
}
