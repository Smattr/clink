// implementation of a string-containing queue

#pragma once

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
 * \param sq Queue to operate on
 * \param str String to add
 * \returns 0 on success if the string was added, EALREADY if the string had
 *   already been in the queue previously, or another errno on failure
 */
int str_queue_push(str_queue_t *sq, const char *str);

/** remove a string from the head of the queue
 *
 * \param sq Queue to operate on
 * \param str [out] String that was popped
 * \returns 0 if a string was popped, ENOMSG if the queue was empty, or an errno
 *   on failure
 */
int str_queue_pop(str_queue_t *sq, char **str);

/** clear and deallocate a queue
 *
 * \param sq Queue to destroy
 */
void str_queue_free(str_queue_t **sq);
