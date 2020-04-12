#pragma once

#include <clink/symbol.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/// opaque type of an iterator
typedef struct clink_iter clink_iter_t;

/** check if the iterator is not exhausted
 *
 * \param it The iterator to examine
 * \returns true if the iterator is non-empty
 */
bool clink_iter_has_next(const clink_iter_t *it);

/** yield the next string from an iterator
 *
 * The iterator passed to this function should be one that yields strings,
 * otherwise it will fail.
 *
 * \param it The iterator structure to operate on
 * \param yielded [out] The next string in the iterator on success
 * \returns 0 on success or an errno on failure
 */
int clink_iter_next_str(clink_iter_t *it, const char **yielded);

/** yield the next symbol from an iterator
 *
 * The iterator passed to this function should be one that yields symbols,
 * otherwise it will fail.
 *
 * \param it The iterator structure to operate on
 * \param yielded [out] The next symbol in the iterator on success
 * \returns 0 on success or an errno on failure
 */
int clink_iter_next_symbol(clink_iter_t *it, const clink_symbol_t **yielded);

/** clean up and deallocate an iterator
 *
 * The iterator, *it, will be set to NULL following a call to this function.
 *
 * \param it Iterator to clean up
 */
void clink_iter_free(clink_iter_t **it);

#ifdef __cplusplus
}
#endif
