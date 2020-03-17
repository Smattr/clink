#include <errno.h>
#include "list.h"
#include <stdlib.h>

int list_append(list_t *l, void *item) {

  // do we need to expand the list?
  if (l->size == l->capacity) {
    size_t capacity = l->capacity == 0 ? 1 : l->capacity * 2;
    void **p = realloc(l->data, sizeof(l->data[0]) * capacity);
    if (p == NULL)
      return ENOMEM;
    l->data = p;
    l->capacity = capacity;
  }

  l->data[l->size] = item;
  l->size++;

  return 0;
}

void list_free(list_t *l, void (*free_item)(void* item)) {
  for (size_t i = 0; i < l->size; i++) {
    if (free_item != NULL) {
      free_item(l->data[i]);
    } else {
      free(l->data[i]);
    }
  }
  free(l->data);
  l->size = 0;
  l->capacity = 0;
}
