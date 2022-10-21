#include "find_repl.h"
#include "debug.h"
#include "find_me.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

char *find_repl(void) {

  char *repl = NULL;

  // can we locate ourselves?
  char *me = find_me();
  if (me == NULL)
    goto done;

  // find slice of this containing the directory we live in
  char *last_slash = strrchr(me, '/');
  if (last_slash == NULL)
    goto done;

  // construct an absolute path to the REPL assuming it is adjacent to us
  if (ERROR(asprintf(&repl, "%.*s/clink-repl", (int)(last_slash - me), me) < 0))
    goto done;

  // reject this if it does not seem usable
  if (access(repl, R_OK | X_OK) < 0) {
    free(repl);
    repl = NULL;
    goto done;
  }

done:
  free(me);

  return repl;
}
