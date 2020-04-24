#include <errno.h>
#include "path.h"
#include <stddef.h>
#include <string.h>

int dirname(const char *path, char **dir) {

  if (path == NULL)
    return EINVAL;

  if (dir == NULL)
    return EINVAL;

  // we assume the path we have been given is absolute
  if (path[0] != '/')
    return EINVAL;

  // if we are at the root, it simply has itself as a dir name
  if (is_root(path)) {
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
