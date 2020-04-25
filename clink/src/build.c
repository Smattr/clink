#include <assert.h>
#include "build.h"
#include <clink/clink.h>
#include <errno.h>
#include "option.h"
#include "path.h"
#include <pthread.h>
#include "sigint.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "work_queue.h"

/// mutual exclusion mechanism for writing to the database
pthread_mutex_t db_lock;

/// add a symbol to the Clink database
static int add_symbol(clink_db_t *db, const clink_symbol_t *symbol) {

  int rc = pthread_mutex_lock(&db_lock);
  if (rc)
    return rc;

  rc = clink_db_add_symbol(db, symbol);

  (void)pthread_mutex_unlock(&db_lock);

  return rc;
}

/// add a content line to the Clink database
static int add_line(clink_db_t *db, const char *path, unsigned long lineno,
    const char *line) {

  int rc = pthread_mutex_lock(&db_lock);
  if (rc)
    return rc;

  rc = clink_db_add_line(db, path, lineno, line);

  (void)pthread_mutex_unlock(&db_lock);

  return rc;
}

/// print progress indication
__attribute__((format(printf, 1, 2)))
static void progress(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vprintf(fmt, ap);
  va_end(ap);
  printf("\n");
}

/// print an error message
__attribute__((format(printf, 1, 2)))
static void error(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
  printf("\n");
}

/// Debug printf. This is implemented as a macro to avoid expensive varargs
/// handling when we are not in debug mode.
#define DEBUG(args...) \
  do { \
    if (option.debug) { \
      progress(args); \
    } \
  } while (0)

/// drain a work queue, processing its entries into the database
static int process(clink_db_t *db, work_queue_t *wq) {

  assert(db != NULL);
  assert(wq != NULL);

  int rc = 0;

  for (;;) {

    // get an item from the work queue
    task_t t;
    rc = work_queue_pop(wq, &t);

    // if we have exhausted the work queue, we are done
    if (rc == ENOMSG) {
      progress("exiting...");
      rc = 0;
      break;
    }

    if (rc) {
      error("failed to pop work queue: %s", strerror(rc));
      break;
    }

    assert(t.path != NULL);

    // generate a friendlier name for the source path
    char *display = NULL;
    if ((rc = disppath(t.path, &display))) {
      free(t.path);
      error("failed to make %s relative: %s", t.path, strerror(rc));
      break;
    }

    switch (t.type) {

      // a file to be parsed
      case PARSE: {

        // remove anything related to the file we are about to parse
        clink_db_remove(db, t.path);

        // enqueue this file for reading, as we know we will need its contents
        if ((rc = work_queue_push_for_read(wq, t.path))) {
          error("failed to queue %s for reading: %s", display, strerror(rc));
          break;
        }

        clink_iter_t *it = NULL;

        // assembly
        if (is_asm(t.path)) {
          progress("parsing asm file %s", display);
          rc = clink_parse_asm(&it, t.path);

        // C/C++
        } else {
          assert(is_c(t.path));
          progress("parsing C/C++ file %s", display);
          const char **argv = (const char**)option.cxx_argv;
          rc = clink_parse_c(&it, t.path, option.cxx_argc, argv);

        }

        if (rc == 0) {
          // parse all symbols and add them to the database
          while (clink_iter_has_next(it)) {

            const clink_symbol_t *symbol = NULL;
            if ((rc = clink_iter_next_symbol(it, &symbol)))
              break;
            assert(symbol != NULL);

            DEBUG("adding symbol %s:%lu:%lu:%s", symbol->path, symbol->lineno,
              symbol->colno, symbol->name);
            if ((rc = add_symbol(db, symbol)))
              break;
          }
        }

        clink_iter_free(&it);

        if (rc)
          error("failed to parse %s: %s", display, strerror(rc));

        break;
      }

      // a file to be read and syntax highlighted
      case READ: {
        progress("syntax highlighting %s", display);
        clink_iter_t *it = NULL;
        rc = clink_vim_highlight(&it, t.path);

        if (rc == 0) {
          // retrieve all lines and add them to the database
          unsigned long lineno = 1;
          while (clink_iter_has_next(it)) {

            const char *line = NULL;
            if ((rc = clink_iter_next_str(it, &line)))
              break;

            if ((rc = add_line(db, t.path, lineno, line)))
              break;

            ++lineno;
          }
        }

        clink_iter_free(&it);

        if (rc) {

          // If the user hit Ctrl+C, Vim may have been SIGINTed causing it to fail
          // cryptically. If it looks like this happened, give the user a less
          // confusing message.
          if (sigint_pending()) {
            error("failed to read %s: received SIGINT", display);

          } else {
            error("failed to read %s: %s", display, strerror(rc));
          }
        }

        break;
      }

    }

    free(display);
    free(t.path);

    if (rc)
      break;

    // check if we have been SIGINTed and should finish up
    if (sigint_pending()) {
      progress("saw SIGINT; exiting...");
      break;
    }
  }

  return rc;
}

int build(clink_db_t *db) {

  assert(db != NULL);

  int rc = 0;

  // create a mutex for protecting database accesses
  if ((rc = pthread_mutex_init(&db_lock, NULL))) {
    fprintf(stderr, "failed to create mutex: %s\n", strerror(rc));
    return rc;
  }

  // setup a work queue to manage our tasks
  work_queue_t *wq = NULL;
  if ((rc = work_queue_new(&wq))) {
    fprintf(stderr, "failed to create work queue: %s\n", strerror(rc));
    goto done;
  }

  // add our source paths to the work queue
  for (size_t i = 0; i < option.src_len; ++i) {
    rc = work_queue_push_for_parse(wq, option.src[i]);

    // ignore duplicate paths
    if (rc == EALREADY)
      rc = 0;

    if (rc) {
      fprintf(stderr, "failed to add %s to work queue: %s\n", option.src[i],
        strerror(rc));
      goto done;
    }
  }

  // suppress SIGINT, so that we do not get interrupted midway through a
  // database write and corrupt it
  if ((rc = sigint_block())) {
    fprintf(stderr, "failed to block SIGINT: %s\n", strerror(rc));
    goto done;
  }

  if ((rc = process(db, wq)))
    goto done;

done:
  (void)sigint_unblock();
  work_queue_free(&wq);
  (void)pthread_mutex_destroy(&db_lock);

  return rc;
}
