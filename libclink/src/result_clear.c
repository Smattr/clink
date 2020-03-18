#include <assert.h>
#include <clink/symbol.h>
#include <stdlib.h>

void clink_result_clear(struct clink_result *r) {

  assert(r != NULL);

  clink_symbol_clear(&r->symbol);

  free(r->context);
  r->context = NULL;
}
