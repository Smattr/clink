#include <assert.h>
#include <clink/vim.h>
#include <errno.h>
#include "run.h"
#include <stdio.h>

int clink_vim_open(const char *filename, unsigned long lineno,
    unsigned long colno) {

  assert(filename != NULL);

  // check line number is valid
  if (lineno == 0)
    return ERANGE;

  // check column number is valid
  if (colno == 0)
    return ERANGE;

  // construct a directive telling Vim to jump to the given position
  char cursor[128];
  snprintf(cursor, sizeof(cursor), "+call cursor(%lu,%lu)", lineno, colno);

  // construct a argument vector to invoke Vim
  char const *argv[] = { "vim", cursor, filename, NULL };

  // run it
  return run(argv, false);
}
