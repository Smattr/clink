#include "../../common/compiler.h"
#include "debug.h"
#include "get_environ.h"
#include "term.h"
#include <assert.h>
#include <clink/vim.h>
#include <errno.h>
#include <fcntl.h>
#include <spawn.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

// To understand the code that follows, it is useful to know several
// characteristics of how Vim renders a file to the terminal:
//
//   1. The content of the file itself is preceded by a status line. At the end
//      of this, Vim emits "\033[1;1H" to move back to the top left to begin
//      emitting the file content.
//
//   2. If Vim needs to wrap a line, it will emit "\033[<line>;1H" midway
//      through a line to adjust the cursor position. If we fake large enough
//      terminal dimensions to Vim, we should never see these sequences, except
//      as noted in (3).
//
//   3. A run of blank lines is emitted by Vim as a single "\033[<line>;1H" to
//      move the cursor across this range.
//
//   4. Formatting state is not always reset at the end of a line. That is, if
//      a given line changed colours or style (bold, italic, …), the line may
//      not end in "\033[0m" if the next line begins in the same colouring and
//      style.
//
//   5. The content of the file is followed by a series of mode adjusting
//      sequences (optionally enabling mouse events, changing repainting, …).
//      These are mostly from the “private sequences” space of ANSI escape
//      codes.
//
//   6. Trailing blank lines in the file are not emitted by Vim at all, as they
//      do not need display.

/// learn the number of lines and maximum line width of a text file
static int get_extent(const char *filename, size_t *rows, size_t *columns) {
  assert(filename != NULL);

  FILE *f = fopen(filename, "r");
  if (f == NULL)
    return errno;

  size_t lines = 1;
  size_t width = 0;
  size_t max_width = 0;
  int last = EOF;

  int rc = 0;

  while (true) {

    int c = getc(f);
    if (c == EOF)
      break;
    last = c;

    // is this a Unix end of line?
    if (c == '\n') {
      ++lines;
      if (max_width < width)
        max_width = width;
      width = 0;
      continue;
    }

    // is this a Windows end of line?
    if (c == '\r') {
      int n = getc(f);
      if (n == '\n') {
        ++lines;
        if (max_width < width)
          max_width = width;
        width = 0;
        continue;
      }

      if (n != EOF) {
        if (UNLIKELY(ungetc(n, f) == EOF)) {
          rc = EIO;
          goto done;
        }
      }
    }

    // assume a tab stop is ≤ 8 characters
    if (c == '\t') {
      width += 8;
      continue;
    }

    // Anything else counts as a single character. This over-counts the width
    // required for UTF-8 characters, but that is fine.
    ++width;
  }

  if (max_width < width)
    max_width = width;

  // if the file ended with a newline, we do not count the next (empty) line
  if (last == '\n') {
    assert(lines > 1);
    --lines;
  }

done:
  (void)fclose(f);

  if (LIKELY(rc == 0)) {
    *rows = lines;
    *columns = max_width;
  }

  return rc;
}

/// start Vim, reading and displaying the given file at the given dimensions
static int run_vim(FILE **out, pid_t *pid, const char *filename, size_t rows,
                   size_t columns, size_t top_row) {

  assert(out != NULL);
  assert(pid != NULL);
  assert(filename != NULL);
  assert(columns <= 10000 && "Vim will not render this many columns");
  assert(rows <= 999 && "Vim will not render this many rows");

  // we need one extra row for the Vim statusline
  ++rows;

  // bump the terminal dimensions if they are likely to confuse or impede Vim
  if (rows < 2)
    rows = 2;
  if (columns < 80)
    columns = 80;

  // clamp the top row to a legal value
  if (top_row == 0)
    top_row = 1;

  int rc = 0;
  FILE *output = NULL;
  int devnull = -1;

  posix_spawn_file_actions_t actions;
  if (UNLIKELY((rc = posix_spawn_file_actions_init(&actions))))
    return rc;

  // create a pipe on which we can receive Vim’s rendering of the file
  int fd[2] = {-1, -1};
  if (UNLIKELY((rc = pipe(fd))))
    goto done;

  // set close-on-exec on the read end which the child (Vim) does not need
  {
    int flags = fcntl(fd[0], F_GETFD);
    if (UNLIKELY(fcntl(fd[0], F_SETFD, flags | O_CLOEXEC) == -1)) {
      rc = errno;
      goto done;
    }
  }

  // turn the read end of the pipe into a file handle
  output = fdopen(fd[0], "r");
  if (UNLIKELY(output == NULL)) {
    rc = errno;
    goto done;
  }
  fd[0] = -1;

  // dup the write end of the pipe over Vim’s stdout
  if (UNLIKELY((rc = posix_spawn_file_actions_adddup2(&actions, fd[1],
                                                      STDOUT_FILENO))))
    goto done;

  // dup /dev/null over Vim’s stdin and stderr
  devnull = open("/dev/null", O_RDWR);
  if (UNLIKELY(devnull < 0)) {
    rc = errno;
    goto done;
  }
  if (UNLIKELY((rc = posix_spawn_file_actions_adddup2(&actions, devnull,
                                                      STDIN_FILENO))))
    goto done;
  if (UNLIKELY((rc = posix_spawn_file_actions_adddup2(&actions, devnull,
                                                      STDERR_FILENO))))
    goto done;

  // construct Vim parameter to force terminal height
  char set_rows[sizeof("+set lines=") + 20];
  (void)snprintf(set_rows, sizeof(set_rows), "+set lines=%zu", rows);

  // construct Vim parameter to force terminal width
  char set_columns[sizeof("+set columns=") + 20];
  (void)snprintf(set_columns, sizeof(set_columns), "+set columns=%zu", columns);

  // construct Vim parameter to jump to the given row
  char call_cursor[sizeof("+call cursor(,1)") + 20];
  (void)snprintf(call_cursor, sizeof(call_cursor), "+call cursor(%zu,1)",
                 top_row);

  DEBUG("running Vim with '+set lines=%zu', '+set columns=%zu', '+call "
        "cursor(%zu,1)' on %s",
        rows, columns, top_row, filename);

  // construct a Vim invocation that opens and displays the file and then exits
  char const *argv[] = {
      "vim",
      "-R",                // read-only mode
      "--not-a-term",      // do not check whether std* is a TTY
      "-X",                // do not connect to X server
      "+set nonumber",     // hide line numbers in case the user has them on
      "+set laststatus=0", // hide status footer line
      "+set noruler",      // hide row,column position footer
      "+set nowrap",       // disable text wrapping in case we have long rows
      set_rows,
      set_columns,
      call_cursor,
      "+z",      // scroll cursor row to the top of the buffer
      "+redraw", // force a screen render to happen before exiting
      "+qa!",    // exit with prejudice
      "--",
      filename,
      NULL};

  // spawn Vim
  pid_t p = 0;
  if (UNLIKELY(((rc = posix_spawnp(&p, argv[0], &actions, NULL,
                                   (char *const *)argv, get_environ())))))
    goto done;

  // success
  *out = output;
  output = NULL;
  *pid = p;

done:
  if (devnull >= 0)
    (void)close(devnull);
  if (output != NULL)
    (void)fclose(output);
  if (fd[0] >= 0)
    (void)close(fd[0]);
  if (fd[1] >= 0)
    (void)close(fd[1]);
  (void)posix_spawn_file_actions_destroy(&actions);

  return rc;
}

int clink_vim_read(const char *filename,
                   int (*callback)(void *state, const char *line),
                   void *state) {

  if (UNLIKELY(filename == NULL))
    return EINVAL;

  if (UNLIKELY(callback == NULL))
    return EINVAL;

  int rc = 0;
  term_t *term = NULL;

  // learn the extent (character width and height) of this file so we can lie to
  // Vim and claim we have a terminal of these dimensions to prevent it
  // line-wrapping and/or truncating
  size_t rows = 0;
  size_t columns = 0;
  if ((rc = get_extent(filename, &rows, &columns)))
    goto done;

  DEBUG("%s has %zu rows and %zu columns", filename, rows, columns);

  // Vim has a hard limit of 10000 columns, so if the file is wider than that we
  // just let anything beyond this be invisible
  if (UNLIKELY(columns > 10000)) {
    DEBUG("clamping %zu columns to 10000", columns);
    columns = 10000;
  }

  // Vim has a hard limit of 1000 rows, so subtract 1 for the statusline and
  // move in chunks of 999 rows if we have a file taller than this
  size_t term_rows = rows;
  if (term_rows > 1000) {
    DEBUG("clamping terminal rows from %zu to 1000", term_rows);
    term_rows = 1000;
  }

  // create a virtual terminal
  if (UNLIKELY((rc = term_new(&term, columns, term_rows))))
    goto done;

  for (size_t row = 1; row <= rows;) {

    // if we are beyond the first iteration of this loop, clear terminal
    // contents from the last iteration
    if (row > 1)
      term_reset(term);

    // how many rows can we render in this pass?
    size_t vim_rows = rows - row + 1;
    if (vim_rows > 999)
      vim_rows = 999;

    // ask Vim to render the file
    FILE *vim_stdout = NULL;
    pid_t vim = 0;
    if (UNLIKELY((
            rc = run_vim(&vim_stdout, &vim, filename, vim_rows, columns, row))))
      goto done;

    assert(vim_stdout != NULL && "invalid stream for Vim’s output");
    assert(vim > 0 && "invalid PID for Vim");

    // drain Vim’s output into the virtual terminal
    rc = term_send(term, vim_stdout);

    // clean up after Vim
    {
      (void)fclose(vim_stdout);
      vim_stdout = NULL;

      int status;
      if (UNLIKELY(waitpid(vim, &status, 0) < 0)) {
        if (rc == 0)
          rc = errno;
      } else if (rc == 0) {
        if (WIFEXITED(status)) {
          rc = WEXITSTATUS(status);
        } else {
          rc = status;
        }
      }
      vim = 0;
      if (UNLIKELY(rc != 0))
        goto done;
    }

    // pass terminal lines back to the caller
    for (size_t y = 1; y <= vim_rows; ++y) {

      const char *line = NULL;
      if (UNLIKELY((rc = term_readline(term, y, &line))))
        goto done;

      if (UNLIKELY((rc = callback(state, line))))
        goto done;
    }

    row += vim_rows;
  }

done:
  term_free(&term);

  return rc;
}
