#include "cwd.h"
#include "path.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>

int disppath(const char *path, char **display) {

  if (path == NULL)
    return EINVAL;

  if (display == NULL)
    return EINVAL;

  // normalise the input path
  char *a = realpath(path, NULL);
  if (a == NULL)
    return errno;

  char *d = NULL;
  int rc = 0;

  // get the current working directory
  const char *wd = cwd_get();

  // if the given path was exactly the working directory, describe it as “.”
  if (strcmp(wd, a) == 0) {
    d = strdup(".");
    if (d == NULL)
      rc = ENOMEM;
    goto done;
  }

  // if wd is a prefix of the path, take the suffix as the display
  if (strncmp(wd, a, strlen(wd)) == 0 && a[strlen(wd)] == '/') {
    d = strdup(&a[strlen(wd) + 1]);
    if (d == NULL)
      rc = ENOMEM;
    goto done;
  }

  // otherwise just use the path itself
  d = strdup(a);
  if (d == NULL)
    rc = ENOMEM;

done:
  free(a);

  if (rc == 0)
    *display = d;

  return rc;
}
