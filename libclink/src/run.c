#include "run.h"
#include "../../common/compiler.h"
#include "get_environ.h"
#include <assert.h>
#include <errno.h>
#include <spawn.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

int run(const char **argv) {

  assert(argv != NULL);

  // spawn the child
  pid_t pid;
  char *const *args = (char *const *)argv;
  int rc = posix_spawnp(&pid, argv[0], NULL, NULL, args, get_environ());
  if (rc != 0)
    return rc;

  // wait for it to finish executing
  {
    int status;
    if (UNLIKELY(waitpid(pid, &status, 0) < 0))
      return errno;
    if (!WIFEXITED(status))
      return status;
    return WEXITSTATUS(status);
  }

  return 0;
}
