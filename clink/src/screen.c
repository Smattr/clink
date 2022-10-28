#include "screen.h"
#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>

/// has `screen_init()` been called?
static bool active;

/// terminal width
static size_t columns;

/// terminal height
static size_t rows;

/// state of the terminal prior to init
static struct termios original_termios;

/// signals that we handle
static const int SIGS[] = {SIGINT, SIGTSTP, SIGWINCH};
enum { SIGS_LENGTH = sizeof(SIGS) / sizeof(SIGS[0]) };

/// signal handlers prior to init
static bool handler_setup[SIGS_LENGTH];
static struct sigaction original_handlers[SIGS_LENGTH];

/// channel for communication from signal handlers to `screen_read()`
static int signal_pipe[2] = {-1, -1};

/// signal redirector
static void signal_bounce(int signum) {
  assert(signal_pipe[1] >= 0 && "signal pipe not setup");
  do {
    ssize_t len = write(signal_pipe[1], &signum, sizeof(signum));
    if (len >= 0) {
      assert((size_t)len == sizeof(signum) && "incomplete write");
      break;
    }
  } while (errno == EINTR);
}

static int set_window_size(void) {

  struct winsize ws = {0};
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) < 0)
    return errno;

  rows = ws.ws_row;
  columns = ws.ws_col;

  return 0;
}

int screen_init(void) {
  assert(!active && "double screen_init() calls");

  int rc = 0;

  // drain any pending output so it will not be subject to our termios changes
  (void)fflush(stdout);

  // determine terminal dimensions
  if ((rc = set_window_size()))
    return rc;

  // read terminal characteristics
  if (tcgetattr(STDOUT_FILENO, &original_termios) < 0)
    return errno;

  // End of read-only actions. Anything after this point should attempt to be
  // undone on failure.

  // set terminal characteristics
  {
    struct termios new = original_termios;
    new.c_lflag &= ~ECHO;   // turn off echo
    new.c_lflag &= ~ICANON; // turn off canonical mode
    if (tcsetattr(STDOUT_FILENO, TCSANOW, &new) < 0) {
      rc = errno;
      goto done;
    }
  }

  // setup a pipe for the signal bouncer to write to
  if (pipe(signal_pipe) < 0) {
    rc = errno;
    goto done;
  }

  // install signal handlers
  struct sigaction sig = {.sa_handler = signal_bounce};
  if (sigemptyset(&sig.sa_mask) < 0) {
    rc = errno;
    goto done;
  }
  for (size_t i = 0; i < SIGS_LENGTH; ++i) {
    if (sigaddset(&sig.sa_mask, SIGS[i]) < 0) {
      rc = errno;
      goto done;
    }
  }
  for (size_t i = 0; i < SIGS_LENGTH; ++i) {
    if (sigaction(SIGS[i], &sig, &original_handlers[i]) < 0) {
      rc = errno;
      goto done;
    }
    handler_setup[i] = true;
  }

  // switch to the alternate screen
  printf("\033[?1049h");

  screen_clear();

  // ensure our changes take effect
  fflush(stdout);

  active = true;

done:
  if (rc != 0)
    screen_free();

  return rc;
}

void screen_clear(void) {
  // clear screen and move to upper left
  printf("\033[2J");
}

size_t screen_get_columns(void) { return columns; }

size_t screen_get_rows(void) { return rows; }

event_t screen_read(void) {
  assert(active && "read from screen prior to screen_init()");

  // wait until we have some data on stdin or from the signal bouncer
  fd_set in;
  FD_ZERO(&in);
  FD_SET(STDIN_FILENO, &in);
  FD_SET(signal_pipe[0], &in);
  int nfds = STDIN_FILENO > signal_pipe[0] ? STDIN_FILENO : signal_pipe[0];
  ++nfds;
  while (true) {
    if (select(nfds, &in, NULL, NULL, NULL) >= 0)
      break;
    if (errno == EINTR)
      continue;
    return (event_t){EVENT_ERROR, (uint32_t)errno};
  }

  // priority 1: did we get a signal?
  if (FD_ISSET(signal_pipe[0], &in)) {
    int signum = 0;
    do {
      ssize_t len = read(signal_pipe[0], &signum, sizeof(signum));
      if (len >= 0) {
        assert((size_t)len == sizeof(signum) && "incomplete read");

        // if this was the signal indicating a window resize, update our
        // knowledge of the dimensions
        if (signum == SIGWINCH) {
          int rc = set_window_size();
          if (rc != 0)
            return (event_t){EVENT_ERROR, (uint32_t)rc};
        }

        return (event_t){EVENT_SIGNAL, (uint32_t)signum};
      }
    } while (errno == EINTR);
    return (event_t){EVENT_ERROR, (uint32_t)errno};
  }

  assert(FD_ISSET(STDIN_FILENO, &in));

  // read a character from stdin
  unsigned char buffer[4]; // enough for a UTF-8 character or escape sequence
  ssize_t len = read(STDIN_FILENO, &buffer, sizeof(buffer));
  if (len < 0)
    return (event_t){EVENT_ERROR, errno};

  // construct a key press event
  event_t key = {.type = EVENT_KEYPRESS};
  for (size_t i = 0; i < (size_t)len; ++i)
    key.value |= (uint32_t)buffer[i] << (i * 8);
  return key;
}

void screen_free(void) {

  if (active) {

    // drain anything pending to avoid it coming out once we switch away from
    // the alternate screen
    fflush(stdout);

    screen_clear();

    // switch out of the alternate screen
    printf("\033[?1049l");

    active = false;
  }

  // restore the signal handlers
  for (size_t i = 0; i < SIGS_LENGTH; ++i) {
    if (handler_setup[i])
      (void)sigaction(SIGS[i], &original_handlers[i], NULL);
    handler_setup[i] = false;
  }

  // discard the signal bouncer pipe
  if (signal_pipe[0] >= 0)
    (void)close(signal_pipe[0]);
  if (signal_pipe[1] >= 0)
    (void)close(signal_pipe[1]);
  signal_pipe[0] = signal_pipe[1] = -1;

  // restore the original terminal characteristics
  (void)tcsetattr(STDOUT_FILENO, TCSANOW, &original_termios);

  fflush(stdout);
}

// build with -DMAIN=main to compile a standalone tool for viewing key sequences
int __attribute__((unused)) MAIN(void);
int __attribute__((unused)) MAIN(void) {

  int rc = 0;

  if ((rc = screen_init())) {
    fprintf(stderr, "failed to setup: %s\n", strerror(rc));
    return EXIT_FAILURE;
  }

  printf("dimensions %zu rows, %zu columns\n", screen_get_rows(),
         screen_get_columns());

  while (true) {

    // read a key press
    event_t e = screen_read();

    if (e.type == EVENT_ERROR) {
      rc = (int)e.value;
      goto done;
    }

    // did we get a signal?
    if (e.type == EVENT_SIGNAL) {

      if (e.value == SIGINT)
        break;

      if (e.value == SIGTSTP) {
        screen_free();
        (void)kill(0, SIGTSTP);
        if ((rc = screen_init())) {
          fprintf(stderr, "failed to setup: %s\n", strerror(rc));
          return EXIT_FAILURE;
        }
        printf("resuming from SIGTSTP…\n");
        continue;
      }

      assert(e.value == SIGWINCH);
      printf("saw window resize, dimensions are now %zu rows, %zu columns\n",
             screen_get_rows(), screen_get_columns());
      continue;
    }

    assert(e.type == EVENT_KEYPRESS);
    // display character bytes to the user
    printf("0x%08" PRIx32 "\n", e.value);

    // exit on Ctrl-D
    if (e.value == 0x4)
      break;
  }

done:
  screen_free();

  if (rc != 0) {
    fprintf(stderr, "failed: %s\n", strerror(rc));
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
