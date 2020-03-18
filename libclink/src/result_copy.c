#include <clink/symbol.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

int clink_result_copy(struct clink_result *dst, const struct clink_result *src) {

  int rc = 0;
  char *context = NULL;

  context = strdup(src->context);
  if (context == NULL) {
    rc = errno;
    goto done;
  }

  if ((rc = clink_symbol_copy(&dst->symbol, &src->symbol)))
    goto done;

  // success if we reached here
  dst->context = context;
  context = NULL;

done:
  free(context);

  return rc;
}
