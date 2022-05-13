#include "str_queue.h"
#include "set.h"
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

struct str_queue {

  /// strings we have previously added to the queue
  set_t *seen;

  /// backing memory for the queue
  char **base;
  size_t capacity;

  /// data curently within the backing memory above
  size_t head;
  size_t size;
};

static void check_invariant(const str_queue_t *sq) {
  assert(sq->head <= sq->capacity);
  assert(sq->size <= sq->capacity);
  assert(sq->head + sq->size <= sq->capacity);

  (void)sq;
}

int str_queue_new(str_queue_t **sq) {

  if (sq == NULL)
    return EINVAL;

  str_queue_t *q = calloc(1, sizeof(*q));
  if (q == NULL)
    return ENOMEM;

  int rc = 0;

  if ((rc = set_new(&q->seen)))
    goto done;

done:
  if (rc) {
    str_queue_free(&q);
  } else {
    check_invariant(q);
    *sq = q;
  }

  return rc;
}

int str_queue_push(str_queue_t *sq, const char *str) {

  if (sq == NULL)
    return EINVAL;

  if (str == NULL)
    return EINVAL;

  check_invariant(sq);

  // check if we have already seen this string
  int rc = set_add(sq->seen, &str);
  if (rc)
    return rc;

  // do we need to expand the backing memory?
  if (sq->capacity == sq->head + sq->size) {
    size_t new_capacity = sq->capacity * 2;
    if (new_capacity == 0)
      new_capacity = 4096 / sizeof(sq->base[0]);

    char **b = realloc(sq->base, new_capacity * sizeof(sq->base[0]));
    if (b == NULL)
      return ENOMEM;

    sq->base = b;
    sq->capacity = new_capacity;

    check_invariant(sq);
  }

  {
    size_t tail = sq->head + sq->size;
    assert(tail < sq->capacity);
    sq->base[tail] = strdup(str);
    if (sq->base[tail] == NULL)
      return ENOMEM;
  }

  ++sq->size;

  return 0;
}

size_t str_queue_size(const str_queue_t *sq) {
  assert(sq != NULL);
  return sq->size;
}

int str_queue_pop(str_queue_t *sq, char **str) {

  if (sq == NULL)
    return EINVAL;

  if (str == NULL)
    return EINVAL;

  check_invariant(sq);

  // is the queue empty?
  if (str_queue_size(sq) == 0)
    return ENOMSG;

  *str = sq->base[sq->head];

  // remove the head
  ++sq->head;
  --sq->size;

  return 0;
}

void str_queue_free(str_queue_t **sq) {

  if (sq == NULL || *sq == NULL)
    return;

  for (size_t i = 0; i < str_queue_size(*sq); ++i)
    free((*sq)->base[(*sq)->head + i]);
  free((*sq)->base);

  set_free(&(*sq)->seen);

  free(*sq);
  *sq = NULL;
}
