#include <clink/symbol.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

int clink_symbol_copy(struct clink_symbol *dst, const struct clink_symbol *src) {

  int rc = 0;
  char *name = NULL;
  char *path = NULL;
  char *parent = NULL;

  name = strndup(src->name, src->name_len);
  if (name == NULL) {
    rc = errno;
    goto done;
  }

  path = strdup(src->path);
  if (path == NULL) {
    rc = errno;
    goto done;
  }

  if (src->parent != NULL) {
    parent = strdup(src->parent);
    if (parent == NULL) {
      rc = errno;
      goto done;
    }
  }

  // success if we reached here
  dst->category = src->category;
  dst->name = name;
  dst->name_len = strlen(name);
  dst->path = path;
  dst->lineno = src->lineno;
  dst->colno = src->colno;
  dst->parent = parent;

  name = NULL;
  path = NULL;
  parent = NULL;

done:
  free(parent);
  free(path);
  free(name);

  return rc;
}
