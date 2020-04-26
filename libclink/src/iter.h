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

/** create a new 1-lookahead iterator from a no-lookahead iterator
 *
 * \param it [out] Created 1-lookahead iterator on success
 * \param impl No-lookahead iterator to wrap
 * \returns 0 on success or an errno on failure
 */
__attribute__((visibility("internal")))
int iter_new(clink_iter_t **it, no_lookahead_iter_t *impl);

/** create a new 1-lookahead string iterator from a no-lookahead string iterator
 *
 * \param it [out] Created 1-lookahead string iterator on success
 * \param impl No-lookahead string iterator to wrap
 * \returns 0 on success or an errno on failure
 */
__attribute__((visibility("internal")))
int iter_str_new(clink_iter_t *it, no_lookahead_iter_t *impl);

/** create a new 1-lookahead symbol iterator from a no-lookahead symbol iterator
 *
 * \param it [out] Created 1-lookahead symbol iterator on success
 * \param impl No-lookahead symbol iterator to wrap
 * \returns 0 on success or an errno on failure
 */
__attribute__((visibility("internal")))
int iter_symbol_new(clink_iter_t *it, no_lookahead_iter_t *impl);
