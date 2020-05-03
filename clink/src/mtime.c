#include <errno.h>
#include "path.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>

int mtime(const char *path, uint64_t *timestamp) {

  if (path == NULL)
    return EINVAL;

  if (strcmp(path, "") == 0)
    return EINVAL;

  if (timestamp == NULL)
    return EINVAL;

  struct stat s;
  if (stat(path, &s) < 0)
    return errno;

  *timestamp = (uint64_t)s.st_mtime;
  return 0;
}
