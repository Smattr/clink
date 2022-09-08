#include "path.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>

int disppath(const char *path, char **display) {

  if (path == NULL)
    return EINVAL;

  if (path[0] != '/')
    return EINVAL;

  if (display == NULL)
    return EINVAL;

  char *d = NULL;

  // get the current working directory
  char *wd = NULL;
  int rc = cwd(&wd);
  if (rc)
    goto done;

  // if the given path was exactly the working directory, describe it as “.”
  if (strcmp(wd, path) == 0) {
    d = strdup(".");
    if (d == NULL)
      rc = ENOMEM;
    goto done;
  }

  // if wd is a prefix of the path, take the suffix as the display
  if (strncmp(wd, path, strlen(wd)) == 0 && path[strlen(wd)] == '/') {
    d = strdup(&path[strlen(wd) + 1]);
    if (d == NULL)
      rc = ENOMEM;
    goto done;
  }

  // otherwise just use the path itself
  d = strdup(path);
  if (d == NULL)
    rc = ENOMEM;

done:
  free(wd);

  if (rc == 0)
    *display = d;

  return rc;
}
