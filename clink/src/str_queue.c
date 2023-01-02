#include "str_queue.h"
#include "set.h"
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

struct str_queue {

  /// strings we have previously added to the queue
  set_t *seen;

  /// backing memory for the queue
  const char **base;
  size_t capacity;

  /// data curently within the backing memory above
  size_t head;
  size_t tail;
};

static void check_invariant(const str_queue_t *sq) {

  size_t head = __atomic_load_n(&sq->head, __ATOMIC_ACQUIRE);

  // head should be within the allocated region
  assert(head <= sq->capacity);

  // tail should be within the allocated region
  assert(sq->tail <= sq->capacity);

  // head should precede tail
  assert(head <= sq->tail);

  (void)sq;
  (void)head;
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

  assert(sq->head == 0 &&
         "head and base are out of sync; pop in str_queue phase 1?");

  // check if we have already seen this string
  int rc = set_add(sq->seen, &str);
  if (rc)
    return rc;

  // do we need to expand the backing memory?
  if (sq->capacity == sq->tail) {
    size_t new_capacity = sq->capacity * 2;
    if (new_capacity == 0)
      new_capacity = 4096 / sizeof(sq->base[0]);

    const char **b = realloc(sq->base, new_capacity * sizeof(sq->base[0]));
    if (b == NULL)
      return ENOMEM;

    sq->base = b;
    sq->capacity = new_capacity;

    check_invariant(sq);
  }

  assert(sq->tail < sq->capacity);
  sq->base[sq->tail] = str;
  ++sq->tail;

  return 0;
}

size_t str_queue_size(const str_queue_t *sq) {
  assert(sq != NULL);
  return sq->tail - sq->head;
}

int str_queue_pop(str_queue_t *sq, const char **str) {

  if (sq == NULL)
    return EINVAL;

  if (str == NULL)
    return EINVAL;

  check_invariant(sq);

  // we can read tail unsynchronised because it is not modified since the last
  // multithreading sequence point, but we need to protect our read of head
  size_t head = __atomic_load_n(&sq->head, __ATOMIC_ACQUIRE);
  size_t tail = sq->tail;

  do {
    // is the queue empty?
    if (head == tail)
      return ENOMSG;

    *str = sq->base[head];

    // try to remove the head
  } while (!__atomic_compare_exchange_n(&sq->head, &head, head + 1, false,
                                        __ATOMIC_RELEASE, __ATOMIC_ACQUIRE));

  return 0;
}

void str_queue_free(str_queue_t **sq) {

  if (sq == NULL || *sq == NULL)
    return;

  free((*sq)->base);

  set_free(&(*sq)->seen);

  free(*sq);
  *sq = NULL;
}
