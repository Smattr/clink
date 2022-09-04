#include "test.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

char *test_asprintf(const char *fmt, ...) {

  va_list ap;
  va_start(ap, fmt);

  // create the requested string
  char *s = NULL;
  {
    int r = vasprintf(&s, fmt, ap);
    va_end(ap);
    ASSERT_GE(r, 0);
  }

  // allocate a new cleanup action
  cleanup_t *c = calloc(1, sizeof(*c));
  if (c == NULL)
    free(s);
  ASSERT_NOT_NULL(c);

  // set it up to free the string we just created
  c->function = free;
  c->arg = s;

  // register it
  c->next = cleanups;
  cleanups = c;

  return s;
}
