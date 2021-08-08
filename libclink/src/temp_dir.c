#include "../../common/compiler.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include "temp_dir.h"
#include <unistd.h>

int temp_dir(char **dir) {

  if (UNLIKELY(dir == NULL))
    return EINVAL;

  // check where the environment wants temporary files to go
  const char *TMPDIR = getenv("TMPDIR");
  if (TMPDIR == NULL)
    TMPDIR = "/tmp";

  char *temp;
  if (UNLIKELY(asprintf(&temp, "%s/tmp.XXXXXX", TMPDIR) < 0))
    return errno;

  if (UNLIKELY(mkdtemp(temp) == NULL)) {
    int rc = errno;
    free(temp);
    return rc;
  }

  *dir = temp;
  return 0;
}
