// a thread-safe queue for pending work

#pragma once

/// opaque type for the queue
typedef struct work_queue work_queue_t;

/// items that can be returned from this queue
typedef struct {

  /// type of this task
  enum {
    PARSE, ///< parse the path as ASM/C/C++
    READ,  ///< read and highlight the contents of this path
  } type;

  /// file to process
  char *path;

} task_t;

/** create a new queue
 *
 * \param wq [out] Created queue on success
 * \returns 0 on success or an errno on failure
 */
int work_queue_new(work_queue_t **wq);

/** enqueue a new file or directory for parsing
 *
 * \param wq Queue to operate on
 * \param path File or directory to add
 * \returns 0 if the path was added, EALREADY if the path had already passed
 *   through the queue, or another errno on failure
 */
int work_queue_push_for_parse(work_queue_t *wq, const char *path);

/** enqueue a new file to be read for highlighting
 *
 * \param wq Queue to operate on
 * \param path File to be read
 * \returns 0 if the path was added, EALREADY if the path had already passed
 *   through the queue, or another errno on failure
 */
int work_queue_push_for_read(work_queue_t *wq, const char *path);

/** dequeue a piece of work from the front of the queue
 *
 * \param wq Queue to operate on
 * \param t [out] Task popped from the from of the queue
 * \returns 0 if a task was popped, ENOMSG if the queue was empty, or another
 *   errno on failure
 */
int work_queue_pop(work_queue_t *wq, task_t *t);

/** clear and deallocate a queue
 *
 * \param wq Queue to destroy
 */
void work_queue_free(work_queue_t **wq);
