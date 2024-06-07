#pragma once

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <unistd.h>

/// `pipe` that also sets close-on-exec
static inline int pipe_(int pipefd[2]) {
  assert(pipefd != NULL);

#ifdef __APPLE__
  // macOS does not have `pipe2`, so we need to fall back on `pipe`+`fcntl`.
  // This is racy, but there does not seem to be a way to avoid this.

  // create the pipe
  if (pipe(pipefd) < 0)
    return errno;

  // set close-on-exec
  for (size_t i = 0; i < 2; ++i) {
    const int flags = fcntl(pipefd[i], F_GETFD);
    if (fcntl(pipefd[i], F_SETFD, flags | FD_CLOEXEC) < 0) {
      const int err = errno;
      for (size_t j = 0; j < 2; ++j) {
        (void)close(pipefd[j]);
        pipefd[j] = -1;
      }
      return err;
    }
  }

#else
  if (pipe2(pipefd, O_CLOEXEC) < 0)
    return errno;
#endif

  return 0;
}
