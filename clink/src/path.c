#include <errno.h>
#include "path.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int abspath(const char *path, char **result) {

  if (path == NULL)
    return EINVAL;

  if (result == NULL)
    return EINVAL;

  // if the path is already absolute, we just need to duplicate it
  if (path[0] == '/') {
    *result = strdup(path);
    if (*result == NULL)
      return ENOMEM;
    return 0;
  }

  // otherwise, we need to prepend the current working directory

  char *wd = NULL;
  int rc = cwd(&wd);
  if (rc)
    return rc;

  int r = asprintf(result, "%s/%s", wd, path);
  if (r < 0)
    rc = errno;

  free(wd);
  return rc;
}

int cwd(char **wd) {

  if (wd == NULL)
    return EINVAL;

  char *w = getcwd(NULL, 0);
  if (w == NULL)
    return errno;

  *wd = w;
  return 0;
}
