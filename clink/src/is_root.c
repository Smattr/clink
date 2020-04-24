#include "path.h"
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

bool is_root(const char *path) {

  if (path == NULL)
    return false;

  return strcmp(path, "/") == 0;
}
