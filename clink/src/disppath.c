#include "path.h"
#include <assert.h>
#include <stddef.h>
#include <string.h>

const char *disppath(const char *cur_dir, const char *path) {

  assert(cur_dir != NULL);
  assert(path != NULL);
  assert(path[0] == '/');

  // if cwd is a prefix of the path, take the suffix as the display
  if (strncmp(cur_dir, path, strlen(cur_dir)) == 0 &&
      path[strlen(cur_dir)] == '/')
    return &path[strlen(cur_dir) + 1];

  // otherwise just use the path itself
  return path;
}
