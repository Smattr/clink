#include "../../common/compiler.h"
#include "run.h"
#include <clink/vim.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

int clink_vim_open(const char *filename, unsigned long lineno,
                   unsigned long colno) {

  // check filename is valid
  if (UNLIKELY(filename == NULL))
    return EINVAL;

  // check line number is valid
  if (UNLIKELY(lineno == 0))
    return EINVAL;

  // check column number is valid
  if (UNLIKELY(colno == 0))
    return EINVAL;

  // construct a directive telling Vim to jump to the given position
  char *cursor = NULL;
  if (UNLIKELY(asprintf(&cursor, "+call cursor(%lu,%lu)", lineno, colno) < 0))
    return ENOMEM;

  // construct a argument vector to invoke Vim
  char const *argv[] = {"vim", cursor, filename, NULL};

  // run it
  int rc = run(argv);

  // clean up
  free(cursor);

  return rc;
}
