#include <assert.h>
#include "build.h"
#include <clink/clink.h>
#include "../../common/compiler.h"
#include <errno.h>
#include "option.h"
#include "path.h"
#include <pthread.h>
#include "sigint.h"
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "work_queue.h"

/// Is stdout a tty? Initialised in build().
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

/// print progress indication
__attribute__((format(printf, 2, 3)))
static void progress(unsigned long thread_id, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  flockfile(stdout);

  // move up to this thread’s progress line
  if (smart_progress())
    printf("\033[%luA\033[K", option.threads - thread_id);

  printf("%lu: ", thread_id);
  vprintf(fmt, ap);
  printf("\n");

  // move back to the bottom
  if (smart_progress() && thread_id != option.threads - 1) {
    printf("\033[%luB", option.threads - thread_id - 1);
    fflush(stdout);
  }

  funlockfile(stdout);
  va_end(ap);
}

/// print an error message
__attribute__((format(printf, 2, 3)))
static void error(unsigned long thread_id, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  flockfile(stdout);
  if (smart_progress())
    printf("\033[%luA\033[K", option.threads - thread_id);
  printf("%lu: ", thread_id);
  if (option.colour == ALWAYS)
    printf("\033[31m"); // red
  vprintf(fmt, ap);
  if (option.colour == ALWAYS)
    printf("\033[0m"); // reset
  printf("\n");
  if (smart_progress() && thread_id != option.threads - 1) {
    printf("\033[%luB", option.threads - thread_id - 1);
    fflush(stdout);
  }
  funlockfile(stdout);
  va_end(ap);
}

/// Debug printf. This is implemented as a macro to avoid expensive varargs
/// handling when we are not in debug mode.
#define DEBUG(args...) \
  do { \
    if (UNLIKELY(option.debug)) { \
      progress(thread_id, args); \
    } \
  } while (0)

/// drain a work queue, processing its entries into the database
static int process(unsigned long thread_id, pthread_t *threads, clink_db_t *db,
    work_queue_t *wq) {

  assert(db != NULL);
  assert(wq != NULL);

  int rc = 0;

  while (true) {

    // get an item from the work queue
    task_t t;
    rc = work_queue_pop(wq, &t);

    // if we have exhausted the work queue, we are done
    if (rc == ENOMSG) {
      progress(thread_id, " ");
      rc = 0;
      break;
    }

    if (UNLIKELY(rc)) {
      error(thread_id, "failed to pop work queue: %s", strerror(rc));
      break;
    }

    assert(t.path != NULL);

    // see if we know of this file
    {
      uint64_t hash = 0;
      uint64_t timestamp = 0;
      if (clink_db_find_record(db, t.path, &hash, &timestamp) == 0) {
        // stat the file to see if it has changed
        struct stat st;
        if (stat(t.path, &st) == 0) {
          // if it has not changed since last update, skip it
          if (hash == (uint64_t)st.st_size) {
            if (timestamp == (uint64_t)st.st_mtime) {
              DEBUG("skipping unmodified file %s", t.path);
              free(t.path);
              continue;
            }
          }
        }
      }
    }

    // generate a friendlier name for the source path
    char *display = NULL;
    if (UNLIKELY((rc = disppath(t.path, &display)))) {
      free(t.path);
      error(thread_id, "failed to make %s relative: %s", t.path, strerror(rc));
      break;
    }

    switch (t.type) {

      // a file to be parsed
      case PARSE: {

        // remove anything related to the file we are about to parse
        clink_db_remove(db, t.path);

        // enqueue this file for reading, as we know we will need its contents
        if (UNLIKELY((rc = work_queue_push_for_read(wq, t.path)))) {
          error(thread_id, "failed to queue %s for reading: %s", display,
            strerror(rc));
          break;
        }

        // assembly
        if (is_asm(t.path)) {

          clink_iter_t *it = NULL;
          progress(thread_id, "parsing asm file %s", display);
          rc = clink_parse_asm(&it, t.path);

          if (rc == 0) {
            // parse all symbols and add them to the database
            while (true) {

              const clink_symbol_t *symbol = NULL;
              if ((rc = clink_iter_next_symbol(it, &symbol))) {
                if (LIKELY(rc == ENOMSG)) // exhausted iterator
                  rc = 0;
                break;
              }
              assert(symbol != NULL);

              DEBUG("adding symbol %s:%lu:%lu:%s", symbol->path, symbol->lineno,
                symbol->colno, symbol->name);
              if (UNLIKELY((rc = clink_db_add_symbol(db, symbol))))
                break;
            }
          }

          clink_iter_free(&it);

        // C/C++
        } else {
          assert(is_c(t.path));
          progress(thread_id, "parsing C/C++ file %s", display);
          const char **argv = (const char**)option.cxx_argv;
          rc = clink_parse_c_into(db, t.path, option.cxx_argc, argv);

        }

        if (UNLIKELY(rc))
          error(thread_id, "failed to parse %s: %s", display, strerror(rc));

        break;
      }

      // a file to be read and syntax highlighted
      case READ: {
        progress(thread_id, "syntax highlighting %s", display);
        clink_iter_t *it = NULL;
        rc = clink_vim_highlight(&it, t.path);

        if (LIKELY(rc == 0)) {
          // retrieve all lines and add them to the database
          unsigned long lineno = 1;
          while (true) {

            const char *line = NULL;
            if ((rc = clink_iter_next_str(it, &line))) {
              if (LIKELY(rc == ENOMSG)) // exhausted iterator
                rc = 0;
              break;
            }

            if (UNLIKELY((rc = clink_db_add_line(db, t.path, lineno, line))))
              break;

            ++lineno;
          }
        }

        clink_iter_free(&it);

        if (UNLIKELY(rc)) {

          // If the user hit Ctrl+C, Vim may have been SIGINTed causing it to fail
          // cryptically. If it looks like this happened, give the user a less
          // confusing message.
          if (sigint_pending()) {
            error(thread_id, "failed to read %s: received SIGINT", display);

          } else {
            error(thread_id, "failed to read %s: %s", display, strerror(rc));
          }
        }

        // now we can insert a record for the file
        if (rc == 0) {
          struct stat st;
          if (LIKELY(stat(t.path, &st) == 0)) {
            uint64_t hash = (uint64_t)st.st_size;
            uint64_t timestamp = (uint64_t)st.st_mtime;
            (void)clink_db_add_record(db, t.path, hash, timestamp);
          }
        }

        break;
      }

    }

    free(display);
    free(t.path);

    if (UNLIKELY(rc))
      break;

    // check if we have been SIGINTed and should finish up
    if (UNLIKELY(sigint_pending())) {
      progress(thread_id, "saw SIGINT; exiting...");
      break;
    }
  }

  // Signals are delivered to one arbitrary thread in a multithreaded process.
  // So if we saw a SIGINT, signal the thread before us so that it cascades and
  // is eventually propagated to all threads.
  if (UNLIKELY(sigint_pending())) {
    unsigned long previous = (thread_id == 0 ? option.threads : thread_id) - 1;
    if (previous != thread_id)
      (void)pthread_kill(threads[previous], SIGINT);
  }

  return rc;
}

// a vehicle for passing data to process()
typedef struct {
  unsigned long thread_id;
  pthread_t *threads;
  clink_db_t *db;
  work_queue_t *wq;
} process_args_t;

// trampoline for unpacking the calling convention used by pthreads
static void *process_entry(void *args) {

  // unpack our arguments
  const process_args_t *a = args;
  unsigned long thread_id = a->thread_id;
  pthread_t *threads = a->threads;
  clink_db_t *db = a->db;
  work_queue_t *wq = a->wq;

  int rc = process(thread_id, threads, db, wq);

  return (void*)(intptr_t)rc;
}

// call process() multi-threaded
static int mt_process(clink_db_t *db, work_queue_t *wq) {

  // the total threads is ourselves plus all the others
  assert(option.threads >= 1);
  size_t bg_threads = option.threads - 1;

  // create threads
  pthread_t *threads = calloc(option.threads, sizeof(threads[0]));
  if (UNLIKELY(threads == NULL))
    return ENOMEM;

  // create state for them
  process_args_t *args = calloc(bg_threads, sizeof(args[0]));
  if (UNLIKELY(args == NULL)) {
    free(threads);
    return ENOMEM;
  }

  // set up data for all threads
  for (size_t i = 1; i < option.threads; ++i)
    args[i - 1] = (process_args_t){
      .thread_id = i, .threads = threads, .db = db, .wq = wq };

  // start all threads
  size_t started = 0;
  for (size_t i = 0; i < option.threads; ++i) {
    if (i == 0) {
      threads[i] = pthread_self();
    } else {
      if (pthread_create(&threads[i], NULL, process_entry, &args[i - 1]) != 0)
        break;
    }
    started = i + 1;
  }

  // join in helping with the rest
  int rc = process(0, threads, db, wq);

  // collect other threads
  for (size_t i = 0; i < started; ++i) {

    // skip ourselves
    if (i == 0)
      continue;

    void *ret;
    int r = pthread_join(threads[i], &ret);

    // none of the pthread failure reasons should be possible
    assert(r == 0);
    (void)r;

    if (ret != NULL && rc == 0)
      rc = (int)(intptr_t)ret;
  }

  // clean up memory
  free(args);
  free(threads);

  return rc;
}

int build(clink_db_t *db) {

  assert(db != NULL);

  tty = isatty(STDOUT_FILENO);

  int rc = 0;

  // setup a work queue to manage our tasks
  work_queue_t *wq = NULL;
  if (UNLIKELY((rc = work_queue_new(&wq)))) {
    fprintf(stderr, "failed to create work queue: %s\n", strerror(rc));
    goto done;
  }

  // add our source paths to the work queue
  for (size_t i = 0; i < option.src_len; ++i) {
    rc = work_queue_push_for_parse(wq, option.src[i]);

    // ignore duplicate paths
    if (rc == EALREADY)
      rc = 0;

    if (UNLIKELY(rc)) {
      fprintf(stderr, "failed to add %s to work queue: %s\n", option.src[i],
        strerror(rc));
      goto done;
    }
  }

  // suppress SIGINT, so that we do not get interrupted midway through a
  // database write and corrupt it
  if (UNLIKELY((rc = sigint_block()))) {
    fprintf(stderr, "failed to block SIGINT: %s\n", strerror(rc));
    goto done;
  }

  // set up progress output table
  if (smart_progress()) {
    for (unsigned long i = 0; i < option.threads; ++i)
      printf("%lu:\n", i);
  }

  if (UNLIKELY((rc = option.threads > 1 ? mt_process(db, wq)
                                        : process(0, NULL, db, wq))))
    goto done;

done:
  (void)sigint_unblock();
  work_queue_free(&wq);

  return rc;
}
