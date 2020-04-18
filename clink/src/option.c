#include <assert.h>
#include <errno.h>
#include "option.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

option_t option = {
  .database_path = NULL,
  .src = NULL,
  .src_len = 0,
  .update_database = true,
  .ncurses_ui = true,
  .line_ui = false,
  .threads = 0,
  .colour = AUTO,
  .cxx_argv = NULL,
  .cxx_argc = 0,
};

int set_db_path(void) {

  // if the user set the database path, no need to do anything else
  if (option.database_path != NULL)
    return 0;

  // get our current directory
  char *cwd = getcwd(NULL, 0);
  if (cwd == NULL)
    return errno;

  int rc = 0;

  // start at this current directory
  char *branch = strdup(cwd);
  if (branch == NULL) {
    rc = ENOMEM;
    goto done;
  }

  for (;;) {

    // create a path to a database at this level
    char *candidate = NULL;
    if (asprintf(&candidate, "%s/.clink.db", branch) < 0) {
      rc = ENOMEM;
      goto done;
    }

    // if this exists, we are done
    if (access(candidate, F_OK) == 0) {
      option.database_path = candidate;
      goto done;
    }

    // if we just checked the file system root, give up
    if (strcmp(branch, "") == 0)
      break;

    // otherwise move one directory up and try again
    for (size_t i = strlen(branch) - 1; ; --i) {
      if (branch[i] == '/') {
        branch[i] = '\0';
        break;
      }
      assert(i != 0);
    }
  }

  // if we still did not find a database, default to the current directory
  if (asprintf(&option.database_path, "%s/.clink.db", cwd) < 0) {
    rc = ENOMEM;
    goto done;
  }

done:
  free(branch);
  free(cwd);

  return rc;
}

void clean_up_options(void) {

  free(option.database_path);
  option.database_path = NULL;

  for (size_t i = 0; i < option.src_len; ++i)
    free(option.src[i]);
  free(option.src);
  option.src = NULL;
  option.src_len = 0;

  for (size_t i = 0; i < option.cxx_argc; ++i)
    free(option.cxx_argv[i]);
  free(option.cxx_argv);
  option.cxx_argv = NULL;
  option.cxx_argc = 0;
}
