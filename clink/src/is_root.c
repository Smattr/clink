#include "path.h"
#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

bool is_root(const char *path) {

  if (path == NULL)
    return false;

  // the root directory should be accessible to us, so if this is the root we
  // should be able to resolve it with realpath()
  char *resolved = realpath(path, NULL);
  if (resolved == NULL)
    return false;

  bool root = strcmp(resolved, "/") == 0;
  free(resolved);
  return root;
}
