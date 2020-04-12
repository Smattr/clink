#include <clink/iter.h>
#include <stdbool.h>

struct clink_iter {

  /// iterator-specific implementation of clink_iter_has_next()
  bool (*has_next)(const clink_iter_t *self);

  /// Iterator-specific implementation of clink_iter_next_str(). This should
  /// only be set for iterators that yield strings.
  int (*next_str)(clink_iter_t *self, const char **yielded);

  /// Iterator-specific implementation of clink_iter_next_symbol(). This should
  /// only be set for iterators that yield symbols.
  int (*next_symbol)(clink_iter_t *self, const clink_symbol_t **yielded);

  /// optional state that an iterator may need to track its progress
  void *state;

  /// optional clean up function for iterator contents that will be called from
  /// clink_iter_free()
  void (*free)(clink_iter_t *self);
};
