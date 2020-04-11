#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include "temp_dir.h"
#include <unistd.h>

int temp_dir(char **dir) {

  if (dir == NULL)
    return EINVAL;

  // check where the environment wants temporary files to go
  const char *TMPDIR = getenv("TMPDIR");
  if (TMPDIR == NULL)
    TMPDIR = "/tmp";

  char *temp;
  if (asprintf(&temp, "%s/tmp.XXXXXX", TMPDIR) < 0)
    return errno;

  if (mkdtemp(temp) == NULL) {
    int rc = errno;
    free(temp);
    return rc;
  }

  *dir = temp;
  return 0;
}
