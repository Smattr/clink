#include "cwd.h"
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

static char *cwd;

int cwd_init(void) {
  assert(cwd == NULL);

  cwd = getcwd(NULL, 0);
  if (cwd == NULL)
    return errno;

  return 0;
}

const char *cwd_get(void) {
  assert(cwd != NULL);
  return cwd;
}

void cwd_free(void) {
  free(cwd);
  cwd = NULL;
}
