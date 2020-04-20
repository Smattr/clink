#include <errno.h>
#include "file_queue.h"
#include "path.h"
#include <pthread.h>
#include <stdlib.h>
#include "str_queue.h"
#include "work_queue.h"

struct work_queue {

  /// mutual exlusion mechanism for push/pop
  pthread_mutex_t lock;

  /// directories/files to be parsed
  file_queue_t *to_parse;

  /// files to be read;
  str_queue_t *to_read;
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

  if ((rc = str_queue_new(&w->to_read)))
    goto done;

done:
  if (rc) {
    work_queue_free(&w);
  } else {
    *wq = w;
  }

  return rc;
}

int work_queue_push_for_parse(work_queue_t *wq, const char *path) {

  if (wq == NULL)
    return EINVAL;

  if (path == NULL)
    return EINVAL;

  // do not allow queueing files that aren't ASM/C/C++ for parsing
  if (is_file(path) && !(is_asm(path) || is_c(path)))
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

int work_queue_push_for_read(work_queue_t *wq, const char *path) {

  if (wq == NULL)
    return EINVAL;

  if (path == NULL)
    return EINVAL;

  // refuse to enqueue non-files
  if (!is_file(path))
    return EINVAL;

  int rc = 0;

  if ((rc = pthread_mutex_lock(&wq->lock)))
    return rc;

  if ((rc = str_queue_push(wq->to_read, path)))
    goto done;

done:
  (void)pthread_mutex_unlock(&wq->lock);

  return rc;
}

int work_queue_pop(work_queue_t *wq, task_t *t) {

  if (wq == NULL)
    return EINVAL;

  if (t == NULL)
    return EINVAL;

  int rc = 0;

  if ((rc = pthread_mutex_lock(&wq->lock)))
    return rc;

  // first try to see if we have a file to be read, as we are more likely to
  // have recently been using that and therefore have it in a file system cache
  char *path = NULL;
  if ((rc = str_queue_pop(wq->to_read, &path))) {
    if (rc != ENOMSG)
      goto done;
  }

  if (rc == 0) { // found something
    t->type = READ;
    goto done;
  }
  rc = 0;

  // otherwise, see if we have something to parse
  if ((rc = file_queue_pop(wq->to_parse, &path)))
    goto done;

  t->type = PARSE;

done:
  (void)pthread_mutex_unlock(&wq->lock);

  if (rc == 0)
    t->path = path;

  return rc;
}

void work_queue_free(work_queue_t **wq) {

  if (wq == NULL || *wq == NULL)
    return;

  work_queue_t *w = *wq;

  str_queue_free(&w->to_read);

  file_queue_free(&w->to_parse);

  (void)pthread_mutex_destroy(&w->lock);

  free(*wq);
  *wq = NULL;
}
