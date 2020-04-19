#include <errno.h>
#include "set.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

enum { BUCKETS = 1024 };

typedef struct set_node {
  char *value;
  struct set_node *next;
} set_node_t;

struct set {
  set_node_t *nodes[BUCKETS];
};

int set_new(set_t **s) {

  if (s == NULL)
    return EINVAL;

  set_t *set = calloc(1, sizeof(*set));
  if (set == NULL)
    return ENOMEM;

  *s = set;
  return 0;
}

// MurmurHash from Austin Appleby
static uint64_t hash(const char *key) {

  static const uint64_t seed = 0;

  static const uint64_t m = UINT64_C(0xc6a4a7935bd1e995);
  static const unsigned r = 47;

  size_t len = strlen(key);

  uint64_t h = seed ^ (len * m);

  const unsigned char *data = (const unsigned char*)key;
  const unsigned char *end = data + len / sizeof(uint64_t) * sizeof(uint64_t);

  while (data != end) {

    uint64_t k;
    memcpy(&k, data, sizeof(k));
    data += sizeof(k);

    k *= m;
    k ^= k >> r;
    k *= m;

    h ^= k;
    h *= m;
  }

  const unsigned char *data2 = data;

  switch (len & 7) {
    case 7: h ^= (uint64_t)data2[6] << 48; // fall through
    case 6: h ^= (uint64_t)data2[5] << 40; // fall through
    case 5: h ^= (uint64_t)data2[4] << 32; // fall through
    case 4: h ^= (uint64_t)data2[3] << 24; // fall through
    case 3: h ^= (uint64_t)data2[2] << 16; // fall through
    case 2: h ^= (uint64_t)data2[1] << 8;  // fall through 
    case 1: h ^= (uint64_t)data2[0];
    h *= m;
  }

  h ^= h >> r;
  h *= m;
  h ^= h >> r;

  return h;
}


int set_add(set_t *s, const char *item) {

  if (s == NULL)
    return EINVAL;

  if (item == NULL)
    return EINVAL;

  uint64_t bucket = hash(item) % BUCKETS;

  // check if this item already exists
  for (const set_node_t *n = s->nodes[bucket]; n != NULL; n = n->next) {
    if (strcmp(n->value, item) == 0)
      return EALREADY;
  }

  set_node_t *n = calloc(1, sizeof(*n));
  if (n == NULL)
    return ENOMEM;

  n->value = strdup(item);
  if (n->value == NULL) {
    free(n);
    return ENOMEM;
  }

  n->next = s->nodes[bucket];
  s->nodes[bucket] = n;

  return 0;
}

void set_free(set_t **s) {

  if (s == NULL || *s == NULL)
    return;

  set_t *set = *s;

  for (size_t i = 0; i < BUCKETS; ++i) {
    for (set_node_t *n = set->nodes[i]; n != NULL; ) {
      set_node_t *next = n->next;
      free(n->value);
      free(n);
      n = next;
    }
    set->nodes[i] = NULL;
  }

  free(set);
  *s = NULL;
}
