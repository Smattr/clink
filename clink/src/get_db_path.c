#include <assert.h>
#include <errno.h>
#include "get_db_path.h"
#include "options.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

int get_db_path(char **path) {

  assert(path != NULL);

  // if the user gave us a database path, just use that
  if (options.database_filename != NULL) {
    *path = strdup(options.database_filename);
    if (*path == NULL)
      return ENOMEM;
    return 0;
  }

  // get the current working directory
  char *cwd = getcwd(NULL, 0);
  if (cwd == NULL)
    return errno;

  // walk up the directory tree looking for an existing .clink.db file
  assert(cwd[0] == '/');
  for (;;) {

    // form a path to .clink.db at our current level
    char *candidate = NULL;
    if (asprintf(&candidate, "%s/.clink.db", cwd) < 0) {
      free(cwd);
      return errno;
    }

    // does this file exist?
    struct stat buf;
    if (stat(candidate, &buf) == 0) {
      free(cwd);
      *path = candidate;
      return 0;
    }
    free(candidate);

    // if we are at the root directory, we have tried all possibilities
    if (strcmp(cwd, "/") == 0)
      break;

    // move one directory up
    char *slash = strrchr(cwd, '/');
    if (cwd == slash) {
      // next directory up is the root directory
      cwd[1] = '\0';
    } else {
      *slash = '\0';
    }
  }

  // re-get the current working directory as we overwrote it
  free(cwd);
  cwd = getcwd(NULL, 0);
  if (cwd == NULL)
    return errno;

  // fall back on just $(pwd)/.clink.db
  if (asprintf(path, "%s/.clink.db", cwd) < 0) {
    free(cwd);
    return errno;
  }
  free(cwd);

  return 0;
}
