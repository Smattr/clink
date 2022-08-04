#include "path.h"
#include <errno.h>
#include <stddef.h>
#include <unistd.h>

int cwd(char **wd) {

  if (wd == NULL)
    return EINVAL;

  char *w = getcwd(NULL, 0);
  if (w == NULL)
    return errno;

  *wd = w;
  return 0;
}
