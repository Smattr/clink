#include "db.h"
#include "debug.h"
#include "run.h"
#include <assert.h>
#include <clink/vim.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

/// can this path be successfully passed to Vim?
static bool is_supported(const char *path) {
  assert(path != NULL);

  // There does not seem to be an escaping scheme capable of passing a path
  // through Vim’s `cs add` to where it calls Cscope. Technically we _can_
  // escape a `cscopeprg` value (see `fnameescape`), but the interactions are
  // a little complex. Lets conservatively reject anything unusual for now.
  for (const char *p = path; *p != '\0'; ++p) {
    switch (*p) {
    case 'a' ... 'z':
    case 'A' ... 'Z':
    case '0' ... '9':
    case '_':
    case '.':
    case ',':
    case '+':
    case '-':
    case ':':
    case '@':
    case '%':
    case '/':
      continue;
    }
    return false;
  }
  return true;
}

int clink_vim_open(const char *filename, unsigned long lineno,
                   unsigned long colno, const char *cscopeprg,
                   const clink_db_t *db) {

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
  char *cursor = NULL;
  char *csprg = NULL;
  char *add = NULL;

  // construct a directive telling Vim to jump to the given position
  if (ERROR(asprintf(&cursor, "+call cursor(%lu,%lu)", lineno, colno) < 0)) {
    rc = ENOMEM;
    goto done;
  }

  // assume the user’s preferred editor is Vim, but still locate the path to it
  const char *vim = getenv("VISUAL");
  if (vim == NULL)
    vim = getenv("EDITOR");
  if (vim == NULL)
    vim = "vim";

  // construct a argument vector to invoke Vim
  enum { ARGC_MAX = 8 };
  char const *argv[ARGC_MAX] = {vim, cursor};
  size_t argc = 2;

#define APPEND(str)                                                            \
  do {                                                                         \
    assert(argv[argc] == NULL && "overwriting existing argument");             \
    argv[argc] = (str);                                                        \
    ++argc;                                                                    \
    assert(argc < ARGC_MAX && "exceeding allocated Vim arguments");            \
  } while (0)

  // hide Vim’s informational messages if we will be tweaking its Cscope
  // connection
  if (cscopeprg != NULL || db != NULL)
    APPEND("+set nocscopeverbose");

  // construct a directive to teach Vim a new `cscopeprg`
  if (cscopeprg != NULL) {
    // will we be unable to communicate this to Vim?
    if (ERROR(!is_supported(cscopeprg))) {
      rc = ENOTSUP;
      goto done;
    }

    if (ERROR(asprintf(&csprg, "+set cscopeprg=%s", cscopeprg) < 0)) {
      rc = ENOMEM;
      goto done;
    }

    APPEND(csprg);
  }

  // construct a directive teaching Vim the database
  if (db != NULL) {
    // will we be unable to communicate this to Vim?
    if (ERROR(!is_supported(db->dir) || !is_supported(db->filename))) {
      rc = ENOTSUP;
      goto done;
    }

    if (ERROR(asprintf(&add, "+cs add %s%s", db->dir, db->filename) < 0)) {
      rc = ENOMEM;
      goto done;
    }

    APPEND(add);
  }

  APPEND("--");
  APPEND(filename);

  // run it
  rc = run(argv);

done:
  free(add);
  free(csprg);
  free(cursor);

  return rc;
}
