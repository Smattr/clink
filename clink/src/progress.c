#include "progress.h"
#include "../../common/compiler.h"
#include "option.h"
#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/// total number of items we have to process
static size_t total;

/// number of files we have completed
static size_t done;

/// Is stdout a tty? Initialised in progress_init().
static bool tty;

/// last output line for each thread
static char **status;

/// use ANSI codes to move the cursor around to generate smoother progress
/// output?
static bool smart_progress(void) {

  // do not play ANSI tricks if we are debugging
  if (UNLIKELY(option.debug))
    return false;

  // also do not do it if we are piped into something else
  if (!tty)
    return false;

  // also not if we are using the line-oriented interface because we assume we
  // are being called by Vim that does not expect this progress output
  if (option.line_ui)
    return false;

  return true;
}

static void update(unsigned long thread_id, char *line) {
  assert(line != NULL);

  flockfile(stdout);

  // save this status line
  free(status[thread_id]);
  status[thread_id] = line;

  // move up to this thread’s progress line
  if (smart_progress())
    printf("\033[%luA\033[K", option.threads - thread_id + 1);

  printf("%lu: %s\n", thread_id, status[thread_id]);

  // move back to the bottom
  if (smart_progress()) {
    printf("\033[%luB", option.threads - thread_id);
    fflush(stdout);
  }

  funlockfile(stdout);
}

/** reconstruct the progress output that may have been overwritten
 *
 * Assumes that we are either single-threaded or the caller has flockfile-d
 * stdout.
 */
static void refresh(void) {
  assert(status != NULL);

  // if smart progress is disabled, also disable refresh
  if (!smart_progress())
    return;

  // move up to the first line
  printf("\033[%luA\033[K", option.threads + 1);

  // reshow all the status lines
  for (unsigned long i = 0; i < option.threads; ++i) {
    assert(status[i] != NULL);
    printf("%lu: %s\033[K\n", i, status[i]);
  }

  // reshow the progress
  printf("%zu / %zu (0.00%%)\n", done, total);
}

void progress_status(unsigned long thread_id, const char *fmt, ...) {

  char *buffer = NULL;
  va_list ap;
  va_start(ap, fmt);
  int rc = vasprintf(&buffer, fmt, ap);
  va_end(ap);
  if (UNLIKELY(rc < 0)) {
    // no error reporting
    return;
  }

  update(thread_id, buffer);
}

void progress_warn(unsigned long thread_id, const char *fmt, ...) {

  flockfile(stdout);

  // move to the first line
  if (smart_progress())
    printf("\033[%luA\033[K", option.threads + 1);

  // “warning: [thread <x>] ”
  if (option.colour == ALWAYS)
    printf("\033[33m");
  printf("warning: ");
  if (option.colour == ALWAYS)
    printf("\033[0m");
  if (option.debug)
    printf("[thread %lu] ", thread_id);

  // show the warning itself
  va_list ap;
  va_start(ap, fmt);
  vprintf(fmt, ap);
  va_end(ap);

  // move down to compensate for the upcoming `refresh` call
  if (smart_progress()) {
    for (unsigned long i = 0; i < option.threads + 2; ++i)
      printf("\n");
  }

  refresh();

  funlockfile(stdout);
}

void progress_error(unsigned long thread_id, const char *fmt, ...) {

  char *base = NULL;
  size_t size = 0;
  FILE *buffer = open_memstream(&base, &size);
  if (UNLIKELY(buffer == NULL)) {
    // no error reporting
    return;
  }

  if (option.colour == ALWAYS)
    fprintf(buffer, "\033[31m"); // red

  {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(buffer, fmt, ap);
    va_end(ap);
  }

  if (option.colour == ALWAYS)
    fprintf(buffer, "\033[0m"); // reset

  (void)fclose(buffer);

  update(thread_id, base);
}

void progress_increment(void) {
  assert(done < total && "progress exceeding total item count");

  flockfile(stdout);

  // move up to the line the progress is on
  if (smart_progress())
    printf("\033[1A\033[K");

  // print a progress update
  ++done;
  printf("%zu / %zu (%.02f%%)\n", done, total, (double)done / total * 100);

  funlockfile(stdout);
}

int progress_init(size_t count) {
  tty = isatty(STDOUT_FILENO);

  int rc = 0;

  // allocate memory to track status updates
  assert(status == NULL && "duplicate progress_init()");
  status = calloc(option.threads, sizeof(status[0]));
  if (UNLIKELY(status == NULL)) {
    rc = ENOMEM;
    goto done;
  }
  for (unsigned long i = 0; i < option.threads; ++i) {
    status[i] = strdup("");
    if (UNLIKELY(status[i] == NULL)) {
      rc = ENOMEM;
      goto done;
    }
  }

  done = 0;
  total = count;

  // set up progress output table
  if (smart_progress()) {
    for (unsigned long i = 0; i < option.threads; ++i)
      printf("%lu:\n", i);
    printf("%zu / %zu (0.00%%)\n", done, total);
  }

done:
  if (rc != 0) {
    if (status != NULL) {
      for (unsigned long i = 0; i < option.threads; ++i)
        free(status[i]);
    }
    free(status);
  }

  return rc;
}

void progress_free(void) {
  if (status != NULL) {
    for (unsigned long i = 0; i < option.threads; ++i)
      free(status[i]);
  }
  free(status);
  status = NULL;
}
