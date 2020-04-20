#include <errno.h>
#include "set.h"
#include "str_queue.h"
#include <stdlib.h>
#include <string.h>

/// a node within the linked list that forms the queue itself
typedef struct node {
  char *value;
  struct node *next;
} node_t;

struct str_queue {

  /// strings we have previously added to the queue
  set_t *seen;

  /// head and tail of the queue itself
  node_t *head;
  node_t *tail;
};

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
    *sq = q;
  }

  return rc;
}

int str_queue_push(str_queue_t *sq, const char *str) {

  if (sq == NULL)
    return EINVAL;

  if (str == NULL)
    return EINVAL;

  // check if we have already seen this string
  int rc = set_add(sq->seen, str);
  if (rc)
    return rc;

  node_t *n = calloc(1, sizeof(*n));
  if (n == NULL)
    return ENOMEM;

  n->value = strdup(str);
  if (n->value == NULL) {
    free(n);
    return ENOMEM;
  }

  if (sq->tail == NULL) {
    sq->head = sq->tail = n;
  } else {
    sq->tail->next = n;
    sq->tail = n;
  }

  return 0;
}

int str_queue_pop(str_queue_t *sq, char **str) {

  if (sq == NULL)
    return EINVAL;

  if (str == NULL)
    return EINVAL;

  // is the queue empty?
  if (sq->head == NULL)
    return ENOMSG;

  *str = sq->head->value;

  // remove the head
  node_t *head = sq->head;
  sq->head = sq->head->next;
  free(head);

  // if the queue only had one element, we need to blank the tail too
  if (head == sq->tail)
    sq->tail = NULL;

  return 0;
}

void str_queue_free(str_queue_t **sq) {

  if (sq == NULL || *sq == NULL)
    return;

  for (node_t *n = (*sq)->head; n != NULL; ) {
    node_t *next = n->next;
    free(n->value);
    free(n);
    n = next;
  }
  (*sq)->head = NULL;
  (*sq)->tail = NULL;

  set_free(&(*sq)->seen);

  free(*sq);
  *sq = NULL;
}
