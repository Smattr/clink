#include <errno.h>
#include "path.h"
#include <stddef.h>
#include <stdio.h>

int join(const char *branch, const char *stem, char **path) {

  if (branch == NULL)
    return EINVAL;

  if (stem == NULL)
    return EINVAL;

  if (path == NULL)
    return EINVAL;

  if (is_root(branch)) {
    if (asprintf(path, "/%s", stem) < 0)
      return errno;
  }

  if (asprintf(path, "%s/%s", branch, stem) < 0)
    return errno;

  return 0;
}
