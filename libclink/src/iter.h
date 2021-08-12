#include <clink/iter.h>

struct clink_iter {

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

typedef struct no_lookahead_iter no_lookahead_iter_t;

/// an iterator that does not provide has_next()
struct no_lookahead_iter {

  /// Iterator-specific implementation of clink_iter_next_str(). This should
  /// only be set for iterators that yield strings.
  int (*next_str)(no_lookahead_iter_t *self, const char **yielded);

  /// Iterator-specific implementation of clink_iter_next_symbol(). This should
  /// only be set for iterators that yield symbols.
  int (*next_symbol)(no_lookahead_iter_t *self, const clink_symbol_t **yielded);

  /// optional state that an iterator may need to track its progress
  void *state;

  /// optional clean up function for iterator contents that will be called from
  /// clink_iter_free()
  void (*free)(no_lookahead_iter_t *self);

};

/** clean up and deallocate a no-lookahead iterator
 *
 * \param it Iterator to clean up
 */
__attribute__((visibility("internal")))
void no_lookahead_iter_free(no_lookahead_iter_t **it);
