#include "work_queue.h"
#include "file_queue.h"
#include "path.h"
#include "str_queue.h"
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>

struct work_queue {

  /// mutual exlusion mechanism for push/pop
  pthread_mutex_t lock;

  /// directories/files to be parsed
  file_queue_t *to_parse;
};

int work_queue_new(work_queue_t **wq) {

  if (wq == NULL)
    return EINVAL;

  work_queue_t *w = calloc(1, sizeof(*w));
  if (w == NULL)
    return ENOMEM;

  int rc = 0;

  if ((rc = pthread_mutex_init(&w->lock, NULL))) {
    free(w);
    return rc;
  }

  if ((rc = file_queue_new(&w->to_parse)))
    goto done;

done:
  if (rc) {
    work_queue_free(&w);
  } else {
    *wq = w;
  }

  return rc;
}

int work_queue_push(work_queue_t *wq, const char *path) {

  if (wq == NULL)
    return EINVAL;

  if (path == NULL)
    return EINVAL;

  // do not allow queueing files that are not ASM/C/C++/DEF for parsing
  if (is_file(path) && !(is_asm(path) || is_c(path) || is_def(path)))
    return EINVAL;

  int rc = 0;

  if ((rc = pthread_mutex_lock(&wq->lock)))
    return rc;

  if ((rc = file_queue_push(wq->to_parse, path)))
    goto done;

done:
  (void)pthread_mutex_unlock(&wq->lock);

  return rc;
}

int work_queue_pop(work_queue_t *wq, char **path) {

  if (wq == NULL)
    return EINVAL;

  if (path == NULL)
    return EINVAL;

  int rc = 0;

  if ((rc = pthread_mutex_lock(&wq->lock)))
    return rc;

  // see if we have something to parse
  if ((rc = file_queue_pop(wq->to_parse, path)))
    goto done;

done:
  (void)pthread_mutex_unlock(&wq->lock);

  return rc;
}

void work_queue_free(work_queue_t **wq) {

  if (wq == NULL || *wq == NULL)
    return;

  work_queue_t *w = *wq;

  file_queue_free(&w->to_parse);

  (void)pthread_mutex_destroy(&w->lock);

  free(*wq);
  *wq = NULL;
}
