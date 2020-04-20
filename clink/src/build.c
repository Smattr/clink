#include <assert.h>
#include "build.h"
#include <clink/clink.h>
#include <errno.h>
#include "option.h"
#include "path.h"
#include "sigterm.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "work_queue.h"

/// print progress indication
__attribute__((format(printf, 1, 2)))
static void progress(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vprintf(fmt, ap);
  va_end(ap);
  printf("\n");
}

/// drain a work queue, processing its entries into the database
static int process(clink_db_t *db, work_queue_t *wq) {

  assert(db != NULL);
  assert(wq != NULL);

  int rc = 0;

  for (size_t i = 1; ; ++i) {

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
      progress("failed to pop work queue: %s", strerror(rc));
      break;
    }

    assert(t.path != NULL);

    switch (t.type) {

      // a file to be parsed
      case PARSE: {

        if (is_asm(t.path)) { // assembly
          progress("parsing asm file %s", t.path);

          // TODO

        } else { // otherwise it should be C/C++
          assert(is_c(t.path));
          progress("parsing C/C++ file %s", t.path);

          //TODO
        }
        break;

      }

      // a file to be read and syntax highlighted
      case READ:
        progress("syntax highlighting %s", t.path);
        //TODO
        break;

    }

    free(t.path);

    // periodically check if we have been SIGTERMed and should finish up
    if (i % 128 == 0) {
      if (sigterm_pending()) {
        progress("saw SIGTERM; exiting...");
        break;
      }
    }
  }

  return rc;
}

int build(clink_db_t *db) {

  assert(db != NULL);

  int rc = 0;

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

  // suppress SIGTERM, so that we do not get interrupted midway through a
  // database write and corrupt it
  if ((rc = sigterm_block())) {
    fprintf(stderr, "failed to block SIGTERM: %s\n", strerror(rc));
    goto done;
  }

  if ((rc = process(db, wq)))
    goto done;

done:
  (void)sigterm_unblock();
  work_queue_free(&wq);

  return rc;
}
