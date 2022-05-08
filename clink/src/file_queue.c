#include "file_queue.h"
#include "path.h"
#include "set.h"
#include "str_queue.h"
#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

struct file_queue {

  /// paths we have previously pushed into our queue
  set_t *pushed;

  /// paths we have enqueued, but not yet opened
  str_queue_t *pending;
};

int file_queue_new(file_queue_t **fq) {

  if (fq == NULL)
    return EINVAL;

  file_queue_t *f = calloc(1, sizeof(*f));
  if (f == NULL)
    return ENOMEM;

  int rc = 0;

  if ((rc = set_new(&f->pushed)))
    goto done;

  if ((rc = str_queue_new(&f->pending)))
    goto done;

done:
  if (rc) {
    file_queue_free(&f);
  } else {
    *fq = f;
  }

  return rc;
}

static int push_file(file_queue_t *fq, const char *path) {

  assert(fq != NULL);
  assert(path != NULL);

  int rc = 0;

  // check if we have previously consumed this path
  if ((rc = set_add(fq->pushed, path)))
    return rc;

  return str_queue_push(fq->pending, path);
}

static int push_dir(file_queue_t *fq, const char *path) {

  assert(fq != NULL);
  assert(path != NULL);

  // open the directory for reading
  DIR *dir = opendir(path);
  if (dir == NULL)
    return errno;

  while (true) {

    // read next entry
    errno = 0;
    struct dirent *entry = readdir(dir);

    // end of directory?
    if (entry == NULL && errno == 0) {
      (void)closedir(dir);
      break;
    }

    // error?
    if (entry == NULL)
      return errno;

    // skip special entries
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
      continue;

    // form an absolute path to this entry
    char *sub = NULL;
    int rc = join(path, entry->d_name, &sub);
    if (rc != 0)
      return rc;

    // if this is a directory, recurse
    if (is_dir(sub)) {
      int r = push_dir(fq, sub);
      free(sub);
      if (r != 0)
        return r;
      continue;
    }

    // if this is a file eligible for parsing, enqueue it
    if (is_file(sub) && (is_asm(sub) || is_c(sub) || is_def(sub))) {
      int r = push_file(fq, sub);
      free(sub);
      if (r != 0 && r != EALREADY)
        return r;
      continue;
    }

    free(sub);
  }

  return 0;
}

int file_queue_push(file_queue_t *fq, const char *path) {

  if (fq == NULL)
    return EINVAL;

  if (path == NULL)
    return EINVAL;

  // ensure this path is readable
  if (access(path, R_OK) < 0)
    return errno;

  return is_dir(path) ? push_dir(fq, path) : push_file(fq, path);
}

int file_queue_pop(file_queue_t *fq, char **path) {

  if (fq == NULL)
    return EINVAL;

  if (path == NULL)
    return EINVAL;

  int rc = 0;

  // remove a path from the pending queue
  char *next = NULL;
  if ((rc = str_queue_pop(fq->pending, &next)))
    return rc;
  assert(next != NULL);

  *path = next;
  return 0;
}

void file_queue_free(file_queue_t **fq) {

  if (fq == NULL || *fq == NULL)
    return;

  file_queue_t *f = *fq;

  str_queue_free(&f->pending);

  set_free(&f->pushed);

  free(*fq);
  *fq = NULL;
}
