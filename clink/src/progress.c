#include "progress.h"
#include "../../common/compiler.h"
#include "option.h"
#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

/// total number of items we have to process
static size_t total;

/// number of files we have completed
static size_t done;

/// Is stdout a tty? Initialised in progress_init().
static bool tty;

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

void progress_status(unsigned long thread_id, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  flockfile(stdout);

  // move up to this thread’s progress line
  if (smart_progress())
    printf("\033[%luA\033[K", option.threads - thread_id + 1);

  printf("%lu: ", thread_id);
  vprintf(fmt, ap);
  printf("\n");

  // move back to the bottom
  if (smart_progress()) {
    printf("\033[%luB", option.threads - thread_id);
    fflush(stdout);
  }

  funlockfile(stdout);
  va_end(ap);
}

void progress_error(unsigned long thread_id, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  flockfile(stdout);
  if (smart_progress())
    printf("\033[%luA\033[K", option.threads - thread_id + 1);
  printf("%lu: ", thread_id);
  if (option.colour == ALWAYS)
    printf("\033[31m"); // red
  vprintf(fmt, ap);
  if (option.colour == ALWAYS)
    printf("\033[0m"); // reset
  printf("\n");
  if (smart_progress()) {
    printf("\033[%luB", option.threads - thread_id);
    fflush(stdout);
  }
  funlockfile(stdout);
  va_end(ap);
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

void progress_init(size_t count) {
  tty = isatty(STDOUT_FILENO);

  done = 0;
  total = count;

  // set up progress output table
  if (smart_progress()) {
    for (unsigned long i = 0; i < option.threads; ++i)
      printf("%lu:\n", i);
    printf("%zu / %zu (0.00%%)\n", done, total);
  }
}
