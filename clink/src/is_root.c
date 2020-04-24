#include <limits.h>
#include "path.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

bool is_root(const char *path) {

  if (path == NULL)
    return false;

  // the root directory should be accessible to us, so if this is the root we
  // should be able to resolve it with realpath()
  char resolved[PATH_MAX];
  if (realpath(path, resolved) == NULL)
    return false;

  return strcmp(resolved, "/") == 0;
}
