#include "../../common/compiler.h"
#include "path.h"
#include <assert.h>
#include <errno.h>
#include <string.h>

int parent(const char *path, char **dir) {

  if (UNLIKELY(path == NULL))
    return EINVAL;

  if (UNLIKELY(dir == NULL))
    return EINVAL;

  // the parent of the root directory is itself
  if (is_root(path)) {
    *dir = strdup("/");
    goto done;
  }

  // find where the stem of this path begins
  const char *stem = strrchr(path, '/');
  if (UNLIKELY(stem == NULL))
    return EINVAL;

  // is the parent of this path the root directory?
  if (path == stem) {
    *dir = strdup("/");
    goto done;
  }

  assert(stem > path);
  *dir = strndup(path, stem - path);

done:
  if (UNLIKELY(*dir == NULL))
    return ENOMEM;
  return 0;
}
