#include "help.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <spawn.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#ifdef __APPLE__
#include <crt_externs.h>
#endif

static char **get_environ(void) {
#ifdef __APPLE__
  // on macOS, environ is not directly accessible
  return *_NSGetEnviron();
#else
  // some platforms fail to expose environ in a header (e.g. FreeBSD), so
  // declare it ourselves and assume it will be available when linking
  extern char **environ;

  return environ;
#endif
}

// these symbols are generated by an xxd translation of clink.1
extern unsigned char clink_1[];
extern unsigned int clink_1_len;

// The approach we take below is writing the manpage to a temporary location and
// then asking man to display it. It would be nice to avoid the temporary file
// and just pipe the manpage to man on stdin. However, man on macOS does not
// seem to support reading from pipes. Since we need a work around for at least
// macOS, we just do it uniformly through a temporary file for all platforms.

int help(void) {

  int rc = 0;

  // find temporary storage space
  const char *TMPDIR = getenv("TMPDIR");
  if (TMPDIR == NULL)
    TMPDIR = "/tmp";

  // create a temporary path
  char *path = NULL;
  if (asprintf(&path, "%s/temp.XXXXXX", TMPDIR) < 0)
    return ENOMEM;

  // create a file there
  int fd = mkostemp(path, O_CLOEXEC);
  if (fd == -1) {
    rc = errno;
    free(path);
    path = NULL;
    goto done;
  }

  // write the manpage to the temporary file
  for (size_t offset = 0; offset < (size_t)clink_1_len;) {
    const ssize_t r = write(fd, &clink_1[offset], (size_t)clink_1_len - offset);
    if (r < 0) {
      if (errno == EINTR)
        continue;
      rc = errno;
      goto done;
    }
    assert((size_t)r <= (size_t)clink_1_len - offset);
    offset += (size_t)r;
  }

  (void)close(fd);
  fd = -1;

  // run man to display the help text
  {
    const char *argv[] = {"man",
#ifdef __linux__
                          "--local-file",
#endif
                          path, NULL};
    char *const *args = (char *const *)argv;
    if ((rc = posix_spawnp(NULL, argv[0], NULL, NULL, args, get_environ())))
      goto done;
  }

  // wait for man to finish
  (void)wait(&(int){0});

  // cleanup
done:
  if (fd >= 0)
    (void)close(fd);
  if (path != NULL)
    (void)unlink(path);
  free(path);

  return rc;
}
