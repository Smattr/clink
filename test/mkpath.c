#include "test.h"
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>

static void my_unlink(void *arg) {
  assert(arg != NULL);
  (void)unlink(arg);
}

char *mkpath(void) {

  // create a directory to contain this path
  char *path = mktempd();

  // construct a path to something we know will not exist inside this
  char *target = aprintf("%s/target", path);

  // allocate a new cleanup action
  cleanup_t *c = calloc(1, sizeof(*c));
  ASSERT_NOT_NULL(c);

  // set it up to delete the path we just constructed
  c->function = my_unlink;
  c->arg = target;

  // register it
  c->next = cleanups;
  cleanups = c;

  return target;
}
