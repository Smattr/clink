#include <clink/iter.h>

struct clink_iter {

  /// iterator-specific implementation of clink_iter_next_symbol()
  int (*next_symbol)(clink_iter_t *self, const clink_symbol_t **yielded);

  /// optional state that an iterator may need to track its progress
  void *state;

  /// optional clean up function for iterator contents that will be called from
  /// clink_iter_free()
  void (*free)(clink_iter_t *self);
};
