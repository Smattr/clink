/// \file
/// \brief Curses-like interface for terminal-based UIs
///
/// An obvious question is “why not Ncurses?”
///
///   1. UTF-8 support. While most of our text originates from source code and
///      is unlikely to contain non-ASCII characters, not supporting UTF-8 in a
///      modern program is unacceptable. Ncursesw is the usual answer to UTF-8
///      support, but in practice it is surprisingly difficult to get a usable
///      version of it on all platforms we support.
///
///   2. Better colour support. Ncurses uses a concept of “colour pairs,”
///      supporting a limited number of these. Mapping terminal colours onto
///      these is haphazard and lossy (there are potentially fewer colour pairs
///      than terminal-supported colours), not to mention inefficient. We can do
///      colour support much simpler by just outputting strings from Vim, who
///      has already done sophisticated colour rendering according to the
///      user’s preferences.
///
///   3. Simpler signals handling and/or lifecycle. Ncurses installs `SIGTSTP`
///      and `SIGWINCH` handlers. Because we run a terminal-based program (Vim)
///      in the midst of having Ncurses active, we need to do some elaborate
///      gymnastics to restore the original signal handlers to avoid confusing
///      Vim, then restore Ncurses’ signal handlers on return from Vim. When
///      doing this ourselves, we can just tear down the entire TUI mechanism on
///      running Vim, and re-init it when we resume after Vim.
///
///   4. Simplicity. There are significant parts of Ncurses that are irrelevant
///      to us (e.g. cursor manipulation). Ncurses also does significant work to
///      avoid repainting the entire screen, something that is no longer
///      relevant on modern terminals.

#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/// type of an event returned from `screen_read`
typedef enum {
  EVENT_ERROR,
  EVENT_KEYPRESS,
  EVENT_SIGNAL,
} event_type_t;

/// return type of `screen_read`
typedef struct {
  event_type_t type; ///< was this event a key, signal, or error?
  uint32_t value;    ///< payload (key, signal number, or errno)
} event_t;

/** setup the terminal for Curses-style output
 *
 * This function must be called before using any of the other functions in this
 * header.
 *
 * \return 0 on success or an errno on failure.
 */
int screen_init(void);

/// blank the screen, clearing all text
void screen_clear(void);

/// get the number of columns in the terminal
size_t screen_get_columns(void);

/// get the number of rows in the terminal
size_t screen_get_rows(void);

/** get a new event
 *
 * This function blocks until there is a key press or a signal is received, or
 * an error occurs. Control characters and chords are returned as they are seen
 * by reading stdin. This means e.g. Ctrl-D comes out as 0x4. Non-ASCII UTF-8
 * characters are also readable naturally this way.
 *
 * \return Event seen
 */
event_t screen_read(void);

/** reverse the setup steps from `screen_init`
 *
 * After calling this function, `screen_init` must be called again before using
 * any of the other functions in this header.
 */
void screen_free(void);

/// character sequence to clear the current line from the cursor to EOL
#define CLRTOEOL "\033[K"

/// print something, updating the display immediately
#define PRINT(args...)                                                         \
  do {                                                                         \
    printf(args);                                                              \
    fflush(stdout);                                                            \
  } while (0)
