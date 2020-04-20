// implementation of a queue containing files and directories

#pragma once

typedef struct file_queue file_queue_t;

/** create a new queue
 *
 * \param fq [out] Created queue on success
 * \returns 0 on success or an errno on failure
 */
int file_queue_new(file_queue_t **fq);

/** add a new file or directory to queue
 *
 * \param fq Queue to operate on
 * \param path File or directory to add
 * \returns 0 on success if the string was added, EALREADY if the path had
 *   already been in the queue previously, or another errno on failure
 */
int file_queue_push(file_queue_t *fq, const char *path);

/** remove a file from the head of the queue
 *
 * \param fq Queue to operate on
 * \param path [out] File that was popped
 * \returns 0 if an entry was popped, ENOMSG if the queue was empty, or an errno
 *   on failure
 */
int file_queue_pop(file_queue_t *fq, char **path);

/** clear and deallocate a queue
 *
 * \param fq Queue to destroy
 */
void file_queue_free(file_queue_t **fq);
