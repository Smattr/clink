#include "../../common/compiler.h"
#include "debug.h"
#include "get_environ.h"
#include <assert.h>
#include <clink/vim.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <spawn.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

  int rc = 0;

  while (true) {

    int c = getc(f);
    if (c == EOF)
      break;

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
                   size_t columns) {

  assert(out != NULL);
  assert(pid != NULL);
  assert(filename != NULL);

  // bump the terminal dimensions if they are likely to confuse or impede Vim
  if (rows < 20)
    rows = 20;
  if (columns < 80)
    columns = 80;

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

  // construct a Vim invocation that opens and displays the file and then exits
  char const *argv[] = {
      "vim",
      "-R",                // read-only mode
      "--not-a-term",      // do not check whether std* is a TTY
      "-X",                // do not connect to X server
      "+set nonumber",     // hide line numbers in case the user has them on
      "+set laststatus=0", // hide status footer line
      "+set noruler",      // hide row,column position footer
      set_rows,
      set_columns,
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

static bool startswith(const char *s, const char *prefix) {
  assert(s != NULL);
  assert(prefix != NULL);
  return strncmp(s, prefix, strlen(prefix)) == 0;
}

static bool isnot8(int c) { return isdigit(c) && c != '8'; }

static void strmove(char *to, const char *from) {
  memmove(to, from, strlen(from) + 1);
}

/// shift out characters from `s` if it points at an irrelevant escape sequence
static bool match_ignore(char *s) {

  assert(s != NULL);
  assert(*s == '\033');

  // is this a "<esc>\[\?\d+[hl]" private sequence?
  if (startswith(s, "\033[?")) {
    for (size_t i = 3;; ++i) {
      if (isdigit(s[i])) {
        // continue
      } else if (s[i] == 'h' || s[i] == 'l') {
        strmove(s, s + i + 1);
        return true;
      } else {
        break;
      }
    }
  }

  // is this the "<esc>>" Normal Keypad sequence?
  static const char NORMAL_KEYPAD[] = "\033>";
  if (startswith(s, NORMAL_KEYPAD)) {
    strmove(s, s + sizeof(NORMAL_KEYPAD) - 1);
    return true;
  }

  // is this a "<esc>[\d+h" Set Mode sequence?
  if (startswith(s, "\033[")) {
    for (size_t i = 2;; ++i) {
      if (isdigit(s[i])) {
        // continue
      } else if (s[i] == 'h') {
        strmove(s, s + i + 1);
        return true;
      } else {
        break;
      }
    }
  }

  return false;
}

/// shift out characters from `s` if it points to a cursor move sequence
static bool match_skip_to(char *s, size_t *skip_to_out, size_t *tab_out) {

  assert(s != NULL);
  assert(*s == '\033');
  assert(skip_to_out != NULL);

  // look for "<esc>\[\d+;\d+H"
  if (!startswith(s, "\033["))
    return false;
  size_t skip_to = 0;
  size_t i;
  for (i = 2; isdigit(s[i]); ++i)
    skip_to = skip_to * 10 + s[i] - '0';
  if (s[i] != ';')
    return false;
  size_t tab = 0;
  for (++i; isdigit(s[i]); ++i)
    tab = tab * 10 + s[i] - '0';
  if (s[i] != 'H')
    return false;

  strmove(s, s + i + 1);
  *skip_to_out = skip_to;
  *tab_out = tab - 1; // -1 because '1' is the left margin

  return true;
}

/// prepend spaces to a string
static int prepend_tab(char **s, size_t *size, size_t tab) {

  assert(s != NULL);
  assert(*s != NULL);
  assert(size != NULL);

  // enlarge the memory for these spaces
  size_t new_size = *size + tab;
  char *new_s = realloc(*s, new_size);
  if (UNLIKELY(new_s == NULL))
    return ENOMEM;

  // shuffle the string content forwards to make room
  strmove(new_s + tab, new_s);

  // insert the indentation
  memset(new_s, ' ', tab);

  *s = new_s;
  *size = new_size;
  return 0;
}

/// add a colour formatting directive to the start of a string
static int prepend_colour(char **s, size_t *size, int colour) {

  assert(s != NULL);
  assert(*s != NULL);
  assert(size != NULL);
  assert(colour >= 0 && colour <= 99);

  // enlarge the backing memory to fit the prefix
  size_t directive = sizeof("\033[m") - 1 + (colour > 9 ? 2 : 1);
  size_t new_size = *size + directive;
  char *new_s = realloc(*s, new_size);
  if (UNLIKELY(new_s == NULL))
    return ENOMEM;

  // shuffle the string content forwards to make room
  strmove(new_s + directive, new_s);

  // insert the formatting directive
  (void)snprintf(new_s, directive, "\033[%d", colour);
  assert(new_s[directive - 1] == '\0' &&
         "snprintf did not write NUL terminator");
  new_s[directive - 1] = 'm';

  *s = new_s;
  *size = new_size;
  return 0;
}

/// add a colour reset directive to the end of a string
static int append_reset(char **s, size_t *size) {

  assert(s != NULL);
  assert(*s != NULL);
  assert(size != NULL);

  // do we need to expand the backing allocation to fit this directive?
  static const char RESET[] = "\033[0m";
  size_t length = strlen(*s);
  if (length + sizeof(RESET) > *size) {
    char *new_s = realloc(*s, length + sizeof(RESET));
    if (UNLIKELY(new_s == NULL))
      return ENOMEM;
    *s = new_s;
    *size = length + sizeof(RESET);
  }

  // if this ends in a Windows line ending, insert before this
  if (length >= 2 && strcmp(*s + length - 2, "\r\n") == 0) {
    (void)snprintf(*s + length - 2, sizeof(RESET) + 2, "%s\r\n", RESET);

    // if this ends in a Unix line ending, insert before this
  } else if (length >= 1 && (*s)[length - 1] == '\n') {
    (void)snprintf(*s + length - 1, sizeof(RESET) + 1, "%s\n", RESET);

    // otherwise, just append it
  } else {
    strcat(*s, RESET);
  }

  return 0;
}

enum {
  FG_DEFAULT = 39, // default foreground colour code
  BG_DEFAULT = 49, // default background colour code
};

int clink_vim_read(const char *filename,
                   int (*callback)(void *state, const char *line),
                   void *state) {

  if (UNLIKELY(filename == NULL))
    return EINVAL;

  if (UNLIKELY(callback == NULL))
    return EINVAL;

  int rc = 0;
  char *line = NULL;    // last received line from Vim
  size_t line_size = 0; // allocated bytes backing `line`
  char *saved = NULL;   // pending data from `line` from previous loop iteration
  FILE *vim_stdout = NULL; // pipe to Vim’s stdout
  pid_t vim = 0;

  // learn the extent (character width and height) of this file so we can lie to
  // Vim and claim we have a terminal of these dimensions to prevent it
  // line-wrapping and/or truncating
  size_t rows = 0;
  size_t columns = 0;
  if ((rc = get_extent(filename, &rows, &columns)))
    goto done;

  DEBUG("%s has %zu rows and %zu columns", filename, rows, columns);

  // ask Vim to render the file
  if (UNLIKELY((rc = run_vim(&vim_stdout, &vim, filename, rows, columns))))
    goto done;

  assert(vim_stdout != NULL && "invalid stream for Vim’s output");
  assert(vim > 0 && "invalid PID for Vim");

  int fg = FG_DEFAULT;  // current foreground colour
  int bg = BG_DEFAULT;  // current background colour
  bool is_bold = false; // is bold on?

  // read lines from Vim, yielding the output to the caller
  size_t lineno = 1;
  size_t skip_to = 0;
  size_t tab = 0;
  for (bool first = true;; first = false) {

    // reset errno in preparation for calling `getline`, so we can distinguish
    // between failure and EOF
    errno = 0;

    // do we have a pending line from last iteration?
    if (saved != NULL) {
      free(line);
      line = saved;
      line_size = strlen(saved) + 1;
      saved = NULL;

      // otherwise read in the next one
    } else if (getline(&line, &line_size, vim_stdout) == -1) {
      if (UNLIKELY((rc = errno)))
        goto done;
      break;
    }

    // if there is a pending indentation, apply it now
    if (tab > 0) {
      if (UNLIKELY((rc = prepend_tab(&line, &line_size, tab))))
        goto done;
      tab = 0;
    }

    // if the formatting from the previous line was not closed by Vim, we need
    // to reapply it to this line
    if (fg != FG_DEFAULT) {
      assert(fg >= 30 && fg < 40);
      if (UNLIKELY((rc = prepend_colour(&line, &line_size, fg))))
        goto done;
    }
    if (bg != BG_DEFAULT) {
      assert(bg >= 40 && bg < 50);
      if (UNLIKELY((rc = prepend_colour(&line, &line_size, bg))))
        goto done;
    }
    if (is_bold) {
      if (UNLIKELY((rc = prepend_colour(&line, &line_size, 1))))
        goto done;
    }

    // the first line contains some status output before ESC[1;1H resetting, so
    // strip this
    if (first) {
      static const char MOVE_ORIGIN[] = "\033[1;1H";
      char *reset = strstr(line, MOVE_ORIGIN);
      if (UNLIKELY(reset == NULL)) {
        DEBUG("failed to find <esc>[1;1H origin jump");
        rc = EBADMSG;
        goto done;
      }
      strmove(line, reset + sizeof(MOVE_ORIGIN) - 1);
    }

    // scan for escape sequences we need to adjust
    for (char *p = line; *p != '\0';) {

      // are we pointing at the start of an escape sequence?
      if (*p == '\033') {

        // if this is a foreground colour switch, update our tracking
        if (p[1] == '[' && p[2] == '3' && isnot8(p[3]) && p[4] == 'm') {
          fg = 30 + p[3] - '0';
          p += sizeof("\033[3.m") - 1;
          continue;
        }

        // if this is a background colour switch, update our tracking
        if (p[1] == '[' && p[2] == '4' && isnot8(p[3]) && p[4] == 'm') {
          bg = 40 + p[3] - '0';
          p += sizeof("\033[4.m") - 1;
          continue;
        }

        // if this is bold enable, update our tracking
        static const char BOLD_ENABLE[] = "\033[1m";
        if (startswith(p, BOLD_ENABLE)) {
          is_bold = true;
          p += sizeof(BOLD_ENABLE) - 1;
          continue;
        }

        // if this is reset, update our tracking
        static const char RESET[] = "\033[0m";
        if (startswith(p, RESET)) {
          fg = FG_DEFAULT;
          bg = BG_DEFAULT;
          is_bold = false;
          p += sizeof(RESET) - 1;
          continue;
        }

        // are we pointing at a sequence that should be ignored?
        if (match_ignore(p))
          continue;

        // are we pointing at a sequence to jump over blank lines?
        {
          size_t skip = 0;
          size_t t = 0;
          if (match_skip_to(p, &skip, &t)) {
            if (UNLIKELY(skip_to != 0)) {
              DEBUG("multiple skip line sequences emitted in line %zu", lineno);
              rc = EBADMSG;
              goto done;
            }
            if (UNLIKELY(skip < lineno)) {
              DEBUG("backwards jump to line %zu column %zu emitted in line %zu",
                    skip, lineno, tab);
              rc = EBADMSG;
              goto done;
            }
            if (skip == lineno) {
              // ignore moves to the same line we are currently on, as this is
              // how Vim deals with non-1-width characters
              continue;
            } else {
              skip_to = skip;
              tab = t;

              // stash the pending line that followed this jump sequence
              assert(saved == NULL && "leaking saved line");
              saved = strdup(p);
              if (UNLIKELY(saved == NULL)) {
                rc = ENOMEM;
                goto done;
              }

              // Terminate the line we just completed. This write is safe
              // because we know the jump sequence occupied at least 6 bytes.
              p[0] = '\n';
              p[1] = '\0';

              break;
            }
          }
        }

        // otherwise we have an unrecognised sequence
        DEBUG("unrecognised sequence <esc>%.*s… on line %zu", 10, p + 1,
              lineno);
        rc = EBADMSG;
        goto done;
      }

      // regular character; continue
      ++p;
    }

    // if formatting was not reset at the end of the line, force this
    if (fg != FG_DEFAULT || bg != BG_DEFAULT || is_bold) {
      if (UNLIKELY((rc = append_reset(&line, &line_size))))
        goto done;
    }

    if ((rc = callback(state, line)))
      goto done;

    ++lineno;

    // did we see any directives indicating following blank lines?
    assert(skip_to == 0 || skip_to >= lineno);
    while (skip_to > lineno) {
      if ((rc = callback(state, "\n")))
        goto done;
      ++lineno;
    }
    skip_to = 0;
  }

  // Vim does not bother emitting the trailing blank lines, either explicitly or
  // as a skip sequence, so handle them ourselves
  while (lineno < rows) {
    if ((rc = callback(state, lineno == rows ? "" : "\n")))
      goto done;
    ++lineno;
  }

done:
  free(saved);
  free(line);
  if (vim_stdout != NULL)
    (void)fclose(vim_stdout);

  if (vim > 0) {

    // wait for Vim to finish executing
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
  }

  return rc;
}
