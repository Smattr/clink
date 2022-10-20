#include "db.h"
#include "debug.h"
#include "run.h"
#include <assert.h>
#include <clink/vim.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

int clink_vim_open(const char *filename, unsigned long lineno,
                   unsigned long colno, const clink_db_t *db) {

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
  char *add = NULL;

  // construct a directive telling Vim to jump to the given position
  if (ERROR(asprintf(&cursor, "+call cursor(%lu,%lu)", lineno, colno) < 0)) {
    rc = ENOMEM;
    goto done;
  }

  // construct a argument vector to invoke Vim
  enum { ARGC_MAX = 8 };
  char const *argv[ARGC_MAX] = {"vim", cursor, "+set nocscopeverbose",
                                "+set cscopeprg=clink-repl"};
  size_t argc = 4;

#define APPEND(str)                                                            \
  do {                                                                         \
    assert(argv[argc] == NULL && "overwriting existing argument");             \
    argv[argc] = (str);                                                        \
    ++argc;                                                                    \
    assert(argc < ARGC_MAX && "exceeding allocated Vim arguments");            \
  } while (0)

  // construct a directive teaching Vim the database
  if (db != NULL) {
    // there does not seem to be an escaping scheme capable of passing a path
    // through Vim’s `cs add` to where it calls Cscope, so reject any path
    // containing unusual characters
    for (const char *p = db->path; *p != '\0'; ++p) {
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
      rc = ENOTSUP;
      goto done;
    }

    if (ERROR(asprintf(&add, "+cs add %s", db->path) < 0)) {
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
  free(cursor);

  return rc;
}
