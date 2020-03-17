// simple dynamically allocated list

#pragma once

#include <stddef.h>

typedef struct {
  void **data;
  size_t size;
  size_t capacity;
} list_t;

__attribute__((visibility("internal")))
int list_append(list_t *l, void *item);

__attribute__((visibility("internal")))
void list_free(list_t *l, void (*free_item)(void *item));
