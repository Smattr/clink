#include <errno.h>
#include <signal.h>
#include "sigterm.h"
#include <stdbool.h>
#include <stddef.h>

static int change(bool block) {

  // create a blank signal set
  sigset_t set;
  if (sigemptyset(&set) < 0)
    return errno;

  // add SIGTERM to it
  if (sigaddset(&set, SIGTERM) < 0)
    return errno;

  // manipulate its handling
  int action = block ? SIG_BLOCK : SIG_UNBLOCK;
  if (sigprocmask(action, &set, NULL) < 0)
    return errno;

  return 0;
}

int sigterm_block(void) {
  return change(true);
}

bool sigterm_pending(void) {

  // retrieve pending signals
  sigset_t set;
  if (sigpending(&set))
    return false;

  // is SIGTERM in the set?
  return sigismember(&set, SIGTERM) == 1;
}

int sigterm_unblock(void) {
  return change(false);
}
