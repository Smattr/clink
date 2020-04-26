#include <assert.h>
#include <clink/iter.h>
#include <errno.h>
#include "iter.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

/// state used by a 1-lookahead wrapper string iterator
typedef struct {

  /// inner no-lookahead iterator
  no_lookahead_iter_t *impl;

  /// next items to be yielded
  char *next[2];

  /// which of [0] or [1] is the next str we should yield?
  size_t active;

} state_t;

/// is the string in the given slot valid?
static bool valid(const state_t *s, size_t slot) {
  assert(slot < sizeof(s->next) / sizeof(s->next[0]));
  return s->next[slot] != NULL;
}

/// reset the given string slot
static void clear(state_t *s, size_t slot) {
  assert(slot < sizeof(s->next) / sizeof(s->next[0]));
  free(s->next[slot]);
  s->next[slot] = NULL;
}

/// clean up and deallocate a state
static void state_free(state_t *s) {

  if (s == NULL)
    return;

  if (s->impl != NULL)
    s->impl->free(s->impl);
  s->impl = NULL;

  for (size_t i = 0; i < sizeof(s->next) / sizeof(s->next[0]); ++i)
    clear(s, i);

  s->active = 0;

  free(s);
}

/// get the slot which is not in use
static size_t inactive(const state_t *s) {
  return 1 - s->active;
}

static bool has_next(const clink_iter_t *it) {

  if (it == NULL)
    return false;

  const state_t *s = it->state;

  return valid(s, s->active);
}

static int next(clink_iter_t *it, const char **yielded) {

  if (it == NULL)
    return EINVAL;

  if (yielded == NULL)
    return EINVAL;

  state_t *s = it->state;

  // if the slot we are pointing at is empty, the iterator is exhausted
  if (!valid(s, s->active))
    return ENOMSG;

  // clear the slot we are not using
  clear(s, inactive(s));

  // refill it from the backing iterator
  const char *n = NULL;
  int rc = s->impl->next_str(s->impl, &n);
  if (rc && rc != ENOMSG)
    return rc;

  if (rc != ENOMSG) {
    assert(n != NULL);
    size_t i = inactive(s);
    s->next[i] = strdup(n);
    if (s->next[i] == NULL)
      return ENOMEM;
  }

  // extract the string to yield
  *yielded = s->next[s->active];

  // flip which slot is active for next time
  s->active = inactive(s);

  return 0;
}

static void my_free(clink_iter_t *it) {

  if (it == NULL)
    return;

  state_t *s = it->state;

  state_free(s);
  it->state = NULL;
}

int iter_str_new(clink_iter_t *it, no_lookahead_iter_t *impl) {

  if (it == NULL)
    return EINVAL;

  if (impl == NULL)
    return EINVAL;

  // is this a non-string iterator?
  if (impl->next_str == NULL)
    return EINVAL;

  clink_iter_t i = { .has_next = has_next, .next_str = next, .free = my_free };

  // allocate state for this iterator
  state_t *s = calloc(1, sizeof(*s));
  if (s == NULL)
    return ENOMEM;

  // populate one of its next slots from the backing iterator
  const char *n = NULL;
  int rc = impl->next_str(impl, &n);
  if (rc && rc != ENOMSG)
    goto done;

  // save the retrieved string to be yielded later
  if (rc != ENOMSG) {
    s->next[s->active] = strdup(n);
    if (s->next[s->active] == NULL) {
      rc = ENOMEM;
      goto done;
    }
  }

  rc = 0;

done:
  if (rc) {
    state_free(s);
  } else {
    s->impl = impl;
    i.state = s;
    *it = i;
  }

  return rc;
}
