#include "debug.h"
#include "run.h"
#include <clink/editor.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

int clink_emacs_open(const char *filename, unsigned long lineno,
                     unsigned long colno) {

  // check filename is valid
  if (ERROR(filename == NULL))
    return EINVAL;

  // check line number is valid
  if (ERROR(lineno == 0))
    return EINVAL;

  // check column number is valid
  if (ERROR(colno == 0))
    return EINVAL;

  int rc = 0;
  char *jump = NULL;

  // construct a directive telling Emacs to jump to the given position
  if (ERROR(asprintf(&jump, "+%lu:%lu", lineno, colno) < 0)) {
    rc = ENOMEM;
    goto done;
  }

  // assume the user’s preferred editor is Emacs, but still locate the path to
  // it
  const char *emacs = getenv("VISUAL");
  if (emacs == NULL)
    emacs = getenv("EDITOR");
  if (emacs == NULL)
    emacs = "emacs";

  // construct a argument vector to invoke Emacs
  char const *argv[] = {emacs, jump, "--", filename, NULL};

  // run it
  rc = run(argv);

done:
  free(jump);

  return rc;
}
