#pragma once

#include <clink/symbol.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CLINK_API
#define CLINK_API __attribute__((visibility("default")))
#endif

/// opaque type of an iterator
typedef struct clink_iter clink_iter_t;

/** yield the next string from an iterator
 *
 * The iterator passed to this function should be one that yields strings,
 * otherwise it will fail.
 *
 * \param it The iterator structure to operate on
 * \param yielded [out] The next string in the iterator on success
 * \return 0 on success or an errno on failure
 */
CLINK_API int clink_iter_next_str(clink_iter_t *it, const char **yielded);

/** yield the next symbol from an iterator
 *
 * The iterator passed to this function should be one that yields symbols,
 * otherwise it will fail.
 *
 * \param it The iterator structure to operate on
 * \param yielded [out] The next symbol in the iterator on success
 * \return 0 on success or an errno on failure
 */
CLINK_API int clink_iter_next_symbol(clink_iter_t *it,
                                     const clink_symbol_t **yielded);

/** clean up and deallocate an iterator
 *
 * The iterator, *it, will be set to NULL following a call to this function.
 *
 * \param it Iterator to clean up
 */
CLINK_API void clink_iter_free(clink_iter_t **it);

#ifdef __cplusplus
}
#endif
