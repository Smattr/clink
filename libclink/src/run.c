#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include "get_environ.h"
#include "run.h"
#include <spawn.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

int run(const char **argv, bool mask_stdout) {

  assert(argv != NULL);

  int rc = 0;
  posix_spawn_file_actions_t fa;

  // A pair of descriptors to the controlling and subordinate ends of a PTY for
  // interacting with the child. If we are not using a PTY, only the subordinate
  // gets used.
  int ctrl_devnull = -1;
  int sub_devnull = -1;

  rc = posix_spawn_file_actions_init(&fa);
  if (rc != 0)
    return rc;

  if (mask_stdout) {

    // We can speed up Vim’s execution by giving it a PTY instead of /dev/null.
    // It is not clear why, but using /dev/null slows Vim down somehow.

    // create a new PTY
    ctrl_devnull = posix_openpt(O_RDWR|O_NOCTTY);
    if (ctrl_devnull == -1) {
      rc = errno;
      goto done;
    }

    // make the PTY usable
    if (grantpt(ctrl_devnull) < 0 || unlockpt(ctrl_devnull) < 0) {
      rc = errno;
      goto done;
    }

    // open the subordinate end that we will give to the child process
    {
      const char *sub_name = ptsname(ctrl_devnull);
      if (sub_name == NULL) {
        rc = errno;
        goto done;
      }
      sub_devnull = open(sub_name, O_RDWR|O_NOCTTY);
    }

    // dup /dev/null over the controlling end of the PTY to discard any output
    // the child emits
    {
      int devnull = open("/dev/null", O_RDWR);
      if (devnull < 0 || dup2(devnull, ctrl_devnull) < 0) {
        rc = errno;
        goto done;
      }
      ctrl_devnull = devnull;
    }

    if (sub_devnull < 0) {
      rc = errno;
      goto done;
    }

    // dup /dev/null over standard streams
    rc = posix_spawn_file_actions_adddup2(&fa, sub_devnull, STDIN_FILENO) ||
         posix_spawn_file_actions_adddup2(&fa, sub_devnull, STDOUT_FILENO) ||
         posix_spawn_file_actions_adddup2(&fa, sub_devnull, STDERR_FILENO);
    if (rc != 0)
      goto done;
  }

  {
    // spawn the child
    pid_t pid;
    char *const *args = (char*const*)argv;
    rc = posix_spawnp(&pid, argv[0], &fa, NULL, args, get_environ());
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
  if (sub_devnull != -1)
    close(sub_devnull);
  if (ctrl_devnull != -1)
    close(ctrl_devnull);
  (void)posix_spawn_file_actions_destroy(&fa);

  return rc;
}
