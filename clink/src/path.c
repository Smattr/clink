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

int dirname(const char *path, char **dir) {

  if (path == NULL)
    return EINVAL;

  if (dir == NULL)
    return EINVAL;

  // we assume the path we have been given is absolute
  if (path[0] != '/')
    return EINVAL;

  // if we are at the root, it simply has itself as a dir name
  if (strcmp(path, "/") == 0) {
    *dir = strdup("/");
    if (*dir == NULL)
      return ENOMEM;
    return 0;
  }

  // otherwise, search backwards for a slash
  size_t slash = strlen(path) - 1;
  while (path[slash] == '/' && slash > 0)
    --slash;
  while (path[slash] != '/' && slash > 0)
    --slash;

  if (slash == 0) {
    *dir = strdup("/");
  } else {
    *dir = strndup(path, slash);
  }
  if (*dir == NULL)
    return ENOMEM;

  return 0;
}
