#include <errno.h>
#include "path.h"
#include <stddef.h>
#include <string.h>

int dirname(const char *path, char **dir) {

  if (path == NULL)
    return EINVAL;

  if (dir == NULL)
    return EINVAL;

  // if we are at the root, it simply has itself as a dir name
  if (is_root(path)) {
    *dir = strdup("/");
    if (*dir == NULL)
      return ENOMEM;
    return 0;
  }

  // treat dir of the empty string as the current directory
  if (strcmp(path, "") == 0) {
    *dir = strdup(".");
    if (*dir == NULL)
      return ENOMEM;
    return 0;
  }

  // ignore trailing slashes
  size_t extent = strlen(path);
  while (extent > 0 && path[extent - 1] == '/')
    --extent;

  // search backwards from there to find the next /
  size_t slash = extent == 0 ? 0 : extent - 1;
  while (slash > 0 && path[slash] != '/')
    --slash;

  if (slash == 0) {
    if (path[0] != '/') {
      // if there were no slashes, use the current directory
      *dir = strdup(".");
    } else {
      // otherwise, we are at the root
      *dir = strdup("/");
    }

  } else {
    *dir = strndup(path, slash);
  }

  if (*dir == NULL)
    return ENOMEM;

  return 0;
}
