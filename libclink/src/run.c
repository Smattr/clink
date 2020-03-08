#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include "run.h"
#include <spawn.h>
#include <stdbool.h>
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

int run(const char **argv, bool mask_stdout) {

  assert(argv != NULL);

  int rc = 0;
  posix_spawn_file_actions_t fa;
  int devnull = -1;

  rc = posix_spawn_file_actions_init(&fa);
  if (rc != 0)
    return rc;

  if (mask_stdout) {

#ifdef __APPLE__
    devnull = open("/dev/null", O_RDWR);
#else
    // On non-Apple platforms we can speed up Vim’s execution by giving it a PTY
    // instead of /dev/null. It is not clear why, but using /dev/null slows Vim
    // down somehow. It is also not clear why this PTY strategy does not work on
    // macOS.
    devnull = posix_openpt(O_RDWR);
#endif

    if (devnull < 0) {
      rc = errno;
      goto done;
    }

    // dup /dev/null over standard streams
    rc = posix_spawn_file_actions_adddup2(&fa, devnull, STDIN_FILENO) ||
         posix_spawn_file_actions_adddup2(&fa, devnull, STDOUT_FILENO) ||
         posix_spawn_file_actions_adddup2(&fa, devnull, STDERR_FILENO);
    if (rc != 0)
      goto done;
  }

  {
    // spawn the child
    pid_t pid;
    rc = posix_spawnp(&pid, argv[0], &fa, NULL, (char*const*)argv, get_environ());
    if (rc != 0)
      goto done;

    // wait for it to finish executing
    if (waitpid(pid, &rc, 0) < 0) {
      rc = errno;
      goto done;
    }
  }

done:
  // clean up
  if (devnull != -1)
    close(devnull);
  (void)posix_spawn_file_actions_destroy(&fa);

  return rc;
}
