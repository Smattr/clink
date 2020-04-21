#include <assert.h>
#include <errno.h>
#include <dirent.h>
#include "file_queue.h"
#include "path.h"
#include "set.h"
#include "str_queue.h"
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

struct file_queue {

  /// paths we have previously pushed into our queue
  set_t *pushed;

  /// paths we have enqueued, but not yet opened
  str_queue_t *pending;

  /// current directory we are reading
  DIR *active;
  char *active_stem;
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

int file_queue_push(file_queue_t *fq, const char *path) {

  if (fq == NULL)
    return EINVAL;

  if (path == NULL)
    return EINVAL;

  // ensure this path is readable
  if (access(path, R_OK) < 0)
    return errno;

  int rc = 0;

  // check if we have previously consumed this path
  if ((rc = set_add(fq->pushed, path)))
    return rc;

  return str_queue_push(fq->pending, path);
}

int file_queue_pop(file_queue_t *fq, char **path) {

  if (fq == NULL)
    return EINVAL;

  if (path == NULL)
    return EINVAL;

  int rc = 0;

  for (;;) {

    // first try to get a new entry from the active directory
    if (fq->active != NULL) {
      errno = 0;
      struct dirent *entry = readdir(fq->active);

      // end of directory?
      if (entry == NULL && errno == 0) {
        (void)closedir(fq->active);
        fq->active = NULL;
        free(fq->active_stem);
        fq->active_stem = NULL;
        continue;
      }

      // error?
      if (entry == NULL) {
        rc = errno;
        break;
      }

      // skip special entries
      if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        continue;

      // form an absolute path to this
      char *next = NULL;
      if ((rc = join(fq->active_stem, entry->d_name, &next)))
        break;

      // if this is a directory, enqueue it for later
      if (is_dir(next)) {
        int r = file_queue_push(fq, next);
        free(next);
        if (r != 0 && r != EALREADY) {
          rc = r;
          break;
        }
        continue;
      }

      // if this is a file and eligible for parsing, return it
      if (is_file(next) && (is_asm(next) || is_c(next))) {
        *path = next;
        break;
      }

      // otherwise, we need to try again
      free(next);
      continue;

    }

    // remove a path from the pending queue
    char *next = NULL;
    if ((rc = str_queue_pop(fq->pending, &next)))
      break;
    assert(next != NULL);

    // if this is a file, just return it
    if (is_file(next)) {
      *path = next;
      break;
    }

    // if this is a directory, make it active
    if (is_dir(next)) {
      fq->active = opendir(next);
      if (fq->active == NULL) {
        rc = errno;
        free(next);
        break;
      }
      fq->active_stem = next;
      continue;
    }

    free(next);
  }

  return rc;
}

void file_queue_free(file_queue_t **fq) {

  if (fq == NULL || *fq == NULL)
    return;

  file_queue_t *f = *fq;

  if (f->active)
    (void)closedir(f->active);
  f->active = NULL;

  free(f->active_stem);
  f->active_stem = NULL;

  str_queue_free(&f->pending);

  set_free(&f->pushed);

  free(*fq);
  *fq = NULL;
}
