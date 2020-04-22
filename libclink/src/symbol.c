#include <clink/symbol.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

int clink_symbol_copy(clink_symbol_t *restrict dst,
    const clink_symbol_t *restrict src) {

  if (dst == NULL)
    return EINVAL;

  if (src == NULL)
    return EINVAL;

  memset(dst, 0, sizeof(*dst));
  int rc = 0;

  dst->category = src->category;

  if (src->name != NULL) {
    dst->name = strdup(src->name);
    if (dst->name == NULL) {
      rc = ENOMEM;
      goto done;
    }
  }

  if (src->path != NULL) {
    dst->path = strdup(src->path);
    if (dst->path == NULL) {
      rc = ENOMEM;
      goto done;
    }
  }

  dst->lineno = src->lineno;
  dst->colno = src->colno;

  if (src->parent != NULL) {
    dst->parent = strdup(src->parent);
    if (dst->parent == NULL) {
      rc = ENOMEM;
      goto done;
    }
  }

  if (src->context != NULL) {
    dst->context = strdup(src->context);
    if (dst->context == NULL) {
      rc = ENOMEM;
      goto done;
    }
  }

done:
  if (rc)
    clink_symbol_clear(dst);

  return rc;
}

void clink_symbol_clear(clink_symbol_t *s) {

  if (s == NULL)
    return;

  free(s->name);
  s->name = NULL;

  free(s->parent);
  s->parent = NULL;

  free(s->context);
  s->context = NULL;
}
