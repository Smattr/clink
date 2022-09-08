#include "path.h"
#include <errno.h>
#include <stddef.h>
#include <string.h>

int disppath(const char *cur_dir, const char *path, char **display) {

  if (cur_dir == NULL)
    return EINVAL;

  if (path == NULL)
    return EINVAL;

  if (path[0] != '/')
    return EINVAL;

  if (display == NULL)
    return EINVAL;

  int rc = 0;
  char *d = NULL;

  // if cwd is a prefix of the path, take the suffix as the display
  if (strncmp(cur_dir, path, strlen(cur_dir)) == 0 &&
      path[strlen(cur_dir)] == '/') {
    d = strdup(&path[strlen(cur_dir) + 1]);
    if (d == NULL)
      rc = ENOMEM;
    goto done;
  }

  // otherwise just use the path itself
  d = strdup(path);
  if (d == NULL)
    rc = ENOMEM;

done:
  if (rc == 0)
    *display = d;

  return rc;
}
