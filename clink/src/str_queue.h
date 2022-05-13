// implementation of a string-containing queue

#pragma once

#include <stddef.h>

/// opaque pointer to a queue structure
typedef struct str_queue str_queue_t;

/** create a new queue
 *
 * \param sq [out] Created queue on success
 * \returns 0 on success or an errno on failure
 */
int str_queue_new(str_queue_t **sq);

/** add a new string to queue
 *
 * It is assumed nothing has been popped from the queue when you call this
 * function. That is, operation on a `str_queue_t` is expected to proceed in two
 * distinct phases: (1) push elements, (2) pop elements. Interleaving push and
 * pop steps is not supported.
 *
 * \param sq Queue to operate on
 * \param str String to add
 * \returns 0 on success if the string was added, EALREADY if the string had
 *   already been in the queue previously, or another errno on failure
 */
int str_queue_push(str_queue_t *sq, const char *str);

/** number of elements in a queue
 *
 * \param sq Queue to inspect
 * \returns Number of elements in this queue
 */
size_t str_queue_size(const str_queue_t *sq);

/** remove a string from the head of the queue
 *
 * \param sq Queue to operate on
 * \param str [out] String that was popped
 * \returns 0 if a string was popped, ENOMSG if the queue was empty, or an errno
 *   on failure
 */
int str_queue_pop(str_queue_t *sq, const char **str);

/** clear and deallocate a queue
 *
 * \param sq Queue to destroy
 */
void str_queue_free(str_queue_t **sq);
