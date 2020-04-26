#include <assert.h>
#include <clink/iter.h>
#include <clink/symbol.h>
#include <errno.h>
#include "iter.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

/// state used by a 1-lookahead wrapper symbol iterator
typedef struct {

  /// inner no-lookahead iterator
  no_lookahead_iter_t *impl;

  /// next items to be yielded
  clink_symbol_t next[2];

  /// which of [0] or [1] is the next symbol we should yield?
  size_t active;

} state_t;

/// is the symbol in the given slot valid?
static bool valid(const state_t *s, size_t slot) {
  assert(slot < sizeof(s->next) / sizeof(s->next[0]));
  return s->next[slot].name != NULL;
}

/// reset the given symbol slot
static void clear(state_t *s, size_t slot) {
  assert(slot < sizeof(s->next) / sizeof(s->next[0]));
  clink_symbol_clear(&s->next[slot]);
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

static int next(clink_iter_t *it, const clink_symbol_t **yielded) {

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
  const clink_symbol_t *n = NULL;
  int rc = s->impl->next_symbol(s->impl, &n);
  if (rc && rc != ENOMSG)
    return rc;

  if (rc != ENOMSG) {
    assert(n != NULL);
    size_t i = inactive(s);
    if ((rc = clink_symbol_copy(&s->next[i], n)))
      return rc;
  }

  // extract the string to yield
  *yielded = &s->next[s->active];

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

int iter_symbol_new(clink_iter_t *it, no_lookahead_iter_t *impl) {

  if (it == NULL)
    return EINVAL;

  if (impl == NULL)
    return EINVAL;

  // is this a non-symbol iterator?
  if (impl->next_symbol == NULL)
    return EINVAL;

  clink_iter_t i
    = { .has_next = has_next, .next_symbol = next, .free = my_free };

  // allocate state for this iterator
  state_t *s = calloc(1, sizeof(*s));
  if (s == NULL)
    return ENOMEM;

  // populate one of its next slots from the backing iterator
  const clink_symbol_t *n = NULL;
  int rc = impl->next_symbol(impl, &n);
  if (rc && rc != ENOMSG)
    goto done;

  // save the retrieved symbol to be yielded later
  if (rc != ENOMSG) {
    if ((rc = clink_symbol_copy(&s->next[s->active], n)))
      goto done;
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
