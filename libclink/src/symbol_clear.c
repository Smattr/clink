#include <assert.h>
#include <clink/symbol.h>
#include <stdlib.h>

void clink_symbol_clear(struct clink_symbol *s) {

  assert(s != NULL);

  free(s->name);
  s->name = NULL;

  free(s->path);
  s->path = NULL;

  free(s->parent);
  s->parent = NULL;
}
