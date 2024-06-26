#include "spinner.h"
#include "../../common/compiler.h"
#include "../../common/pipe.h"
#include "option.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

/// position at which to output the progress spinner
static size_t spinner_row;
static size_t spinner_column;

/// channel for the main thread to signal the background thread
static int done[2] = {-1, -1};

/// background worker thread
static pthread_t worker;
static bool worker_inited;

/// output a progress spinner
static void *spin(void *ignored UNUSED) {

  // various positions of the spinner
  static const char *STATES[] = {"⠇", "⠋", "⠙", "⠸", "⠴", "⠦"};

  // Which of the above markers to print next. This is `static` to make the
  // spinner resume from its last position, which leads to a slightly more
  // natural experience.
  static size_t state = 0;

  while (true) {

    // give the main thread a chance to signal us to complete
    assert(done[0] >= 0);
    {
      struct pollfd in[] = {{.fd = done[0], .events = POLLIN}};
      nfds_t nfds = sizeof(in) / sizeof(in[0]);
      if (poll(in, nfds, 100) > 0)
        break;
    }

    // prevent anyone else interleaving stdout writes with us
    flockfile(stdout);

    // move to where the progress spinner should go
    printf("\033[%zu;%zuH", spinner_row, spinner_column);

    // output the spinner itself
    printf("%s", STATES[state]);
    fflush(stdout);
    funlockfile(stdout);

    state = (state + 1) % (sizeof(STATES) / sizeof(STATES[0]));
  }

  return NULL;
}

int spinner_on(size_t row, size_t column) {

  if (worker_inited)
    return EINVAL;

  int rc = 0;

  spinner_row = row;
  spinner_column = column;

  // hide the cursor
  printf("\033[?25l");
  fflush(stdout);

  if (option.animation) {

    assert(done[0] < 0);
    assert(done[1] < 0);
    if (UNLIKELY((rc = pipe_(done))))
      goto done;

    // set the read end of the pipe to be non-blocking so the worker can poll it
    {
      int flags = fcntl(done[0], F_GETFL);
      if (UNLIKELY(fcntl(done[0], F_SETFL, flags | O_NONBLOCK) < 0)) {
        rc = errno;
        goto done;
      }
    }

    assert(!worker_inited);
    if (UNLIKELY(rc = pthread_create(&worker, NULL, spin, NULL)))
      goto done;
    worker_inited = true;

  } else {
    // print static progress indicator
    printf("\033[%zu;%zuH»", spinner_row, spinner_column);
    fflush(stdout);
  }

done:
  if (rc != 0)
    spinner_off();

  return rc;
}

void spinner_off(void) {

  if (worker_inited) {

    // tell the worker to complete
    assert(done[1] >= 0);
    do {
      char ignored = 1;
      ssize_t rc = write(done[1], &ignored, sizeof(ignored));
      if (rc >= 0)
        break;
    } while (errno == EINTR);

    // wait for it to exit
    {
      int UNUSED rc = pthread_join(worker, NULL);
      assert(rc == 0);
    }
  }
  worker_inited = false;

  // show the cursor
  printf("\033[?25h");
  fflush(stdout);

  if (done[0] >= 0)
    (void)close(done[0]);
  if (done[1] >= 0)
    (void)close(done[1]);
  done[0] = done[1] = -1;
}
