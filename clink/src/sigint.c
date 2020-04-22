#include <errno.h>
#include <signal.h>
#include "sigint.h"
#include <stdbool.h>
#include <stddef.h>

static int change(bool block) {

  // create a blank signal set
  sigset_t set;
  if (sigemptyset(&set) < 0)
    return errno;

  // add SIGINT to it
  if (sigaddset(&set, SIGINT) < 0)
    return errno;

  // manipulate its handling
  int action = block ? SIG_BLOCK : SIG_UNBLOCK;
  if (sigprocmask(action, &set, NULL) < 0)
    return errno;

  return 0;
}

int sigint_block(void) {
  return change(true);
}

bool sigint_pending(void) {

  // retrieve pending signals
  sigset_t set;
  if (sigpending(&set))
    return false;

  // is SIGINT in the set?
  return sigismember(&set, SIGINT) == 1;
}

int sigint_unblock(void) {
  return change(false);
}
