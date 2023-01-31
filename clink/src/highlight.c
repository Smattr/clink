#include "highlight.h"
#include "../../common/compiler.h"
#include "option.h"
#include "path.h"
#include "screen.h"
#include "str_queue.h"
#include <assert.h>
#include <clink/clink.h>
#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct {
  clink_db_t *db;
  const char *cur_dir;
  str_queue_t *sources;
  size_t screen_row;
} state_t;

static void *do_work(void *arg) {
  state_t *st = arg;
  assert(st != NULL);

  while (true) {
    const char *path = NULL;
    int rc = str_queue_pop(st->sources, &path);
    if (rc == ENOMSG) {
      break;
    } else if (rc != 0) {
      return (void *)(intptr_t)rc;
    }

    // generate a friendly path
    const char *display_path = disppath(st->cur_dir, path);

    // Update what we are doing. Inline the move and `CLRTOEOL` so we can do
    // it all while holding the stdout lock and avoid racing with the
    // spinner.
    PRINT("\033[%zu;4Hsyntax highlighting %s…%s", st->screen_row, display_path,
          CLRTOEOL);

    // ignore non-fatal failure of highlighting
    (void)clink_vim_read_into(st->db, path);

    // update what we are doing
    PRINT("\033[%zu;4H%s", st->screen_row, CLRTOEOL);
  }

  return NULL;
}

static size_t sub_clamp(size_t a, size_t b) {
  if (b >= a)
    return 1;
  return a - b;
}

int highlight(clink_db_t *db, const char *cur_dir, str_queue_t *sources,
              size_t last_screen_row) {

  assert(db != NULL);
  assert(cur_dir != NULL);
  assert(sources != NULL);
  assert(last_screen_row > 0);

  assert(option.threads >= 1);
  size_t bg_threads = option.threads - 1;

  state_t *args = NULL;
  pthread_t *threads = NULL;
  int rc = 0;

  args = calloc(option.threads, sizeof(args[0]));
  if (UNLIKELY(args == NULL)) {
    rc = ENOMEM;
    goto done;
  }

  // setup state for each thread
  for (size_t i = 0; i < option.threads; ++i) {
    state_t *arg = &args[i];
    arg->db = db;
    arg->cur_dir = cur_dir;
    arg->sources = sources;
    arg->screen_row = sub_clamp(last_screen_row, i);
  }

  // create worker threads
  threads = calloc(bg_threads, sizeof(threads[0]));
  if (UNLIKELY(bg_threads > 0 && threads == NULL)) {
    rc = ENOMEM;
    goto done;
  }
  size_t started = 0;
  for (size_t i = 0; i < bg_threads; ++i) {
    if (pthread_create(&threads[i], NULL, do_work, &args[i + 1]) != 0)
      break;
    ++started;
  }

  // join them ourselves
  (void)do_work(&args[0]);

  // wait for background threads to finish
  for (size_t i = 0; i < started; ++i) {

    void *ret = NULL;
    int r = pthread_join(threads[i], &ret);

    // none of the pthread failure reasons should be possible
    assert(r == 0);
    (void)r;

    if (ret != NULL && rc == 0)
      rc = (int)(intptr_t)ret;
  }

done:
  free(threads);
  free(args);

  return rc;
}
