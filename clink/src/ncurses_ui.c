#include <assert.h>
#include <clink/clink.h>
#include "colour.h"
#include <ctype.h>
#include <curses.h>
#include <errno.h>
#include "ncurses_ui.h"
#include "option.h"
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static struct sigaction original_sigtstp_handler;
static struct sigaction original_sigwinch_handler;

/// did we successfully enable colour mode in Ncurses?
static bool colour;

/// which Clink prompt is the cursor currently on?
static size_t prompt_index;

/// which Clink row entry is currently selected?
static size_t select_index;

/// start of the current screenful of results
static size_t from_row;

/// current position of the cursor
static size_t x, y;

/// state machine for where the cursor is
static enum {
  ST_INPUT,
  ST_ROWSELECT,
  ST_EXITING,
} state = ST_INPUT;

/// text the user has entered to the left of the cursor
static char *left;
static size_t left_size;

/// text the user has entered to the right of the cursor
static char *right;
static size_t right_size;

/// database previously loaded
static clink_db_t *database;

typedef struct {
  clink_symbol_t *rows;
  size_t count;
  size_t size;
} results_t;

/// results of the last query we ran
static results_t results;

/// number of result columns excluding the hot key
enum { COLUMN_COUNT = 4 };

static int format_results(clink_iter_t *it) {

  // free any previous results
  for (size_t i = 0; i < results.count; ++i)
    clink_symbol_clear(&results.rows[i]);
  free(results.rows);
  results.rows = NULL;
  results.count = results.size = 0;

  int rc = 0;

  while (clink_iter_has_next(it)) {

    // retrieve the next symbol
    const clink_symbol_t *symbol = NULL;
    if ((rc = clink_iter_next_symbol(it, &symbol)))
      break;

    // expand results collection if necessary
    if (results.size == results.count) {
      size_t s = results.size == 0 ? 128 : results.size * 2;
      clink_symbol_t *r = realloc(results.rows, s * sizeof(results.rows[0]));
      if (r == NULL) {
        rc = ENOMEM;
        break;
      }
      results.rows = r;
      results.size = s;
    }

    // append the new symbol
    clink_symbol_t *target = &results.rows[results.count];
    if ((rc = clink_symbol_copy(target, symbol)))
      break;
    ++results.count;

    // strip leading white space from the context for neater output
    if (target->context != NULL) {
      while (isspace(target->context[0]))
        memmove(target->context, target->context + 1, strlen(target->context));
    }
  }

  clink_iter_free(&it);

  return rc;
}

// wrappers for each database query

static int find_symbol(const char *query) {

  clink_iter_t *it = NULL;
  int rc = clink_db_find_symbol(database, query, &it);
  if (rc)
    return rc;

  return format_results(it);
}

static int find_definition(const char *query) {

  clink_iter_t *it = NULL;
  int rc = clink_db_find_definition(database, query, &it);
  if (rc)
    return rc;

  return format_results(it);
}

static int find_call(const char *query) {

  clink_iter_t *it = NULL;
  int rc = clink_db_find_call(database, query, &it);
  if (rc)
    return rc;

  return format_results(it);
}

static int find_caller(const char *query) {

  clink_iter_t *it = NULL;
  int rc = clink_db_find_caller(database, query, &it);
  if (rc)
    return rc;

  return format_results(it);
}

static int find_includer(const char *query) {

  clink_iter_t *it = NULL;
  int rc = clink_db_find_includer(database, query, &it);
  if (rc)
    return rc;

  return format_results(it);
}

static const struct searcher {
  const char *prompt;
  int (*handler)(const char *query);
} functions[] = {
  { "Find this C symbol", find_symbol },
  { "Find this definition", find_definition },
  { "Find functions called by this function", find_call },
  { "Find functions calling this function", find_caller },
  { "Find files #including this file", find_includer },
};

static const size_t FUNCTIONS_SZ = sizeof(functions) / sizeof(functions[0]);

static void print_menu(void) {
  move(LINES - FUNCTIONS_SZ, 0);
  for (size_t i = 0; i < FUNCTIONS_SZ; ++i)
    printw("%s: \n", functions[i].prompt);
}

static size_t offset_x(size_t index) {
  assert(index < FUNCTIONS_SZ);
  return strlen(functions[index].prompt) + strlen(": ");
}

static size_t offset_y(size_t index) {
  assert(index < FUNCTIONS_SZ);
  return LINES - FUNCTIONS_SZ + index;
}

static const char HOTKEYS[] =
  "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

static char hotkey(unsigned index) {
  if (index < sizeof(HOTKEYS) - 1)
    return HOTKEYS[index];
  return -1;
}

static size_t usable_rows(void) {
  return (size_t)LINES - FUNCTIONS_SZ - 2 - 1;
}

static void pad(size_t width) {
  for (size_t i = 0; i < width; ++i)
    printw(" ");
}

static size_t digit_count(unsigned long num) {

  if (num == 0)
    return 1;

  size_t count = 0;
  while (num != 0) {
    num /= 10;
    ++count;
  }

  return count;
}

static int print_results(void) {
  assert(from_row == 0 || from_row < results.count);

  // the column headings, excluding the initial hotkey column
  static const char *HEADINGS[COLUMN_COUNT] = { "File", "Function", "Line", "" };

  // the number of rows we can fit is the number of lines on the screen with
  // some room extracted for the column headings, menu and status
  if ((size_t)LINES < FUNCTIONS_SZ + 2 + 1 + 1)
    return -1;
  size_t row_count = usable_rows();
  if (row_count > results.count - from_row) {
    // cannot show more rows than we have
    row_count = results.count - from_row;
  }
  if (row_count > sizeof(HOTKEYS) - 1)
    row_count = sizeof(HOTKEYS) - 1;

  // figure out column widths
  size_t widths[COLUMN_COUNT] = { 0 };
  for (size_t i = 0; i < COLUMN_COUNT; ++i) {
    widths[i] = strlen(HEADINGS[i]);
    // find the maximum width of this column’s results
    for (size_t j = from_row; j < from_row + row_count; ++j) {
      assert(j < results.count);
      const clink_symbol_t *sym = &results.rows[j];
      size_t w =
        i == 0 ? strlen(sym->path) :
        i == 1 ? (sym->parent == NULL ? 0 : strlen(sym->parent)) :
        i == 2 ? digit_count(sym->lineno) :
                 (sym->context == NULL ? 0 : strlen(sym->context));
      if (w > widths[i])
        widths[i] = w;
    }
  }

  // print column headings
  move(0, 0);
  printw("  ");
  for (size_t i = 0; i < COLUMN_COUNT; ++i) {
    printw("%s ", HEADINGS[i]);
    size_t padding = widths[i] - strlen(HEADINGS[i]);
    pad(padding);
  }
  clrtoeol();

  // print the rows
  for (size_t i = 0; i < LINES - FUNCTIONS_SZ - 1 - 1; ++i) {
    move(1 + i, 0);
    if (i < row_count) {
      printw("%c ", hotkey(i));
      for (size_t j = 0; j < COLUMN_COUNT; ++j) {
        const clink_symbol_t *sym = &results.rows[i + from_row];
        size_t padding = widths[j] + 1;
        switch (j) {
          case 0: // file
            padding -= strlen(sym->path);
            printw("%s", sym->path);
            pad(padding);
            break;
          case 1: // function
            padding -= sym->parent == NULL ? 0 : strlen(sym->parent);
            if (sym->parent != NULL)
              printw("%s", sym->parent);
            pad(padding);
            break;
          case 2: // line
            padding -= digit_count(sym->lineno) + 1;
            pad(padding);
            printw("%lu ", sym->lineno);
            break;
          case 3: // context
            if (sym->context != NULL) {
              if (colour) {
                printw_colour(sym->context);
              } else {
                printw_bw(sym->context);
              }
            }
            break;
        }
      }
    }
    clrtoeol();
  }

  // print footer
  move(LINES - FUNCTIONS_SZ - 1, 0);
  printw("* ");
  if (results.count == 0) {
    printw("No results");
  } else {
    printw("Lines %zu-%zu of %zu", from_row + 1, from_row + row_count,
      results.count);
    if (from_row + row_count < results.count) {
      printw(", %zu more - press the space bar to display more",
        results.count - from_row - row_count);
    } else if (from_row > 0) {
      printw(", press the space bar to display the first lines again");
    }
  }
  printw(" *");
  clrtoeol();

  return 0;
}

static void move_to_line_no_blank(size_t target) {
  prompt_index = target;
  x = offset_x(prompt_index);
  y = offset_y(prompt_index);

  // paste the previous contents into the new line
  move(y, x);
  printw("%s%s", left, right);
  x += strlen(left);
}

static void move_to_line(size_t target) {

  // blank the current line
  move(y, offset_x(prompt_index));
  clrtoeol();

  move_to_line_no_blank(target);
}

static int handle_input(void) {

  echo();

  move(y, x);
  int c = getch();

  switch (c) {
    case 4: // Ctrl-D
      state = ST_EXITING;
      return 0;
      break;

    case 10: // enter
      if (strlen(left) > 0 || strlen(right) > 0) {
        char *query = NULL;
        if (asprintf(&query, "%s%s", left, right) < 0)
          return errno;
        int rc = functions[prompt_index].handler(query);
        if (rc)
          return rc;
        from_row = 0;
        select_index = 0;
        print_results();
        if (results.count > 0)
          state = ST_ROWSELECT;
      }
      break;

    case 23: { // Ctrl-W
      while (strlen(left) > 0 && isspace(left[strlen(left) - 1]))
        left[strlen(left) - 1] = '\0';
      while (strlen(left) > 0 && !isspace(left[strlen(left) - 1]))
        left[strlen(left) - 1] = '\0';
      move(y, offset_x(prompt_index));
      printw("%s%s", left, right);
      clrtoeol();
      x = offset_x(prompt_index) + strlen(left);
      move(y, x);
      break;
    }

    case '\t':
      if (results.count > 0)
        state = ST_ROWSELECT;
      break;

    case KEY_LEFT:
      if (strlen(left) > 0) {

        // expand right if necessary
        if (strlen(right) == right_size - 1) {
          char *r = realloc(right, right_size * 2);
          if (r == NULL)
            return ENOMEM;
          right = r;
          right_size *= 2;
        }

        // insert the new character
        memmove(right + 1, right, strlen(right) + 1);
        right[0] = left[strlen(left) - 1];

        // remove it from the left side
        left[strlen(left) - 1] = '\0';

        x--;
      }
      break;

    case KEY_RIGHT:
      if (strlen(right) > 0) {

        // expand left if necessary
        if (strlen(left) == left_size - 1) {
          char *l = realloc(left, left_size * 2);
          if (l == NULL)
            return ENOMEM;
          left = l;
          left_size *= 2;
        }

        // insert the new character
        strncat(left, right, 1);

        // remove it from the right side
        memmove(right, right + 1, strlen(right) + 1);

        x++;
      }
      break;

    case KEY_UP:
      if (prompt_index > 0)
        move_to_line(prompt_index - 1);
      break;

    case KEY_DOWN:
      if (prompt_index < FUNCTIONS_SZ - 1)
        move_to_line(prompt_index + 1);
      break;

    case KEY_HOME:

      // expand right if necessary
      while (strlen(left) + strlen(right) > right_size - 1) {
        char *r = realloc(right, right_size * 2);
        if (r == NULL)
          return ENOMEM;
        right = r;
        right_size *= 2;
      }

      // make room for text to be added
      memmove(right + strlen(left), right, strlen(right) + 1);

      // add the text
      memcpy(right, left, strlen(left));

      // clear the text we just transferred
      left[0] = '\0';

      x = offset_x(prompt_index);
      break;

    case KEY_END:

      // expand left if necessary
      while (strlen(left) + strlen(right) > left_size - 1) {
        char *l = realloc(left, left_size * 2);
        if (l == NULL)
          return ENOMEM;
        left = l;
        left_size *= 2;
      }

      // append the right hand text
      strcat(left, right);

      // clear the text we just transferred
      right[0] = '\0';

      x = offset_x(prompt_index) + strlen(left);
      break;

    case KEY_PPAGE:
      move_to_line(0);
      break;

    case KEY_NPAGE:
      move_to_line(FUNCTIONS_SZ - 1);
      break;

    case 127: // Backspace on macOS
    case KEY_BACKSPACE:
      if (strlen(left) > 0) {
        left[strlen(left) - 1] = '\0';
        x--;
      }
      move(y, x);
      printw("%s", right);
      clrtoeol();
      break;

    case KEY_DC:
      if (strlen(right) > 0) {
        memmove(right, right + 1, strlen(right) + 1);
        printw("%s", right);
        clrtoeol();
      }
      break;

    case KEY_RESIZE:
      endwin();
      clear();
      print_menu();
      if (results.count > 0)
        print_results();
      move_to_line_no_blank(prompt_index);
      break;

    default:
      x++;

      // expand left if necessary
      if (strlen(left) + 1 > left_size - 1) {
        char *l = realloc(left, left_size * 2);
        if (l == NULL)
          return ENOMEM;
        left = l;
        left_size *= 2;
      }

      // append the new character
      size_t end = strlen(left);
      left[end] = c;
      left[end + 1] = '\0';

      if (strlen(right) > 0)
        printw("%s", right);
  }

  return 0;
}

static int handle_select(void) {
  assert(state == ST_ROWSELECT);

  noecho();

  assert(select_index >= from_row);
  if (select_index - from_row + 1 > usable_rows()) {
    // The selected row is out of visible range. This can happen if the terminal
    // window resized while we were not in select mode.
    select_index = from_row;
  }
  move(select_index - from_row + 1, 0);
  int c = getch();

  switch (c) {

    case 4: //* Ctrl-D
      state = ST_EXITING;
      return 0;

/* XXX: egregious abuse of interstice to provide a parameter for function-like
 * label hotkey_select below. At least we can use scoping to contain the damage.
 */
{
__builtin_unreachable();
int base;

    case '0' ... '9':
      base = '0';
      goto hotkey_select;

    case 'a' ... 'z':
      base = 'a' - 10 /* '0' - '9' */;
      goto hotkey_select;

    case 'A' ... 'Z':
      base = 'A' - 10 /* '0' - '9' */ - 26 /* 'a' - 'z' */;
      goto hotkey_select;

hotkey_select:
      if (from_row + c - base < results.count) {
        select_index = from_row + c - base;
        goto enter;
      }
      break;
}

    case 10: { /* enter */
enter:
      def_prog_mode();
      endwin();

      // Restore the SIGTSTP handler we had prior to starting up ncurses.
      // Ncurses registers a SIGTSTP handler that it leaves in place until
      // process exit. This means that when we exec Vim below, if the user
      // suspends Vim (^Z) the SIGTSTP to Clink is masked by the ncurses
      // handler. A consequence of this is that the Clink menu is not
      // repainted when Vim exits. By restoring the original handler, we
      // can claw our way back to regular TTY behaviour.
      struct sigaction curses_tstp;
      int read_tstp = sigaction(SIGTSTP, &original_sigtstp_handler,
        &curses_tstp);

      // Blasted ncurses does the same thing with the SIGWINCH handler. As a
      // result, resizing the terminal window while Vim is open causes ncurses
      // to take incorrect actions. Vim notices the window has resized but that
      // it is missing a SIGWINCH, realises someone else is driving the boat and
      // freaks out and quits.
      struct sigaction curses_winch;
      int read_winch = sigaction(SIGWINCH, &original_sigwinch_handler,
        &curses_winch);

      int rc = clink_vim_open(results.rows[select_index].path,
        results.rows[select_index].lineno, results.rows[select_index].colno);

      reset_prog_mode();

      // restore ncurses’ handlers
      if (read_tstp == 0)
        (void)sigaction(SIGTSTP, &curses_tstp, NULL);
      if (read_winch == 0)
        (void)sigaction(SIGWINCH, &curses_winch, NULL);

      refresh();
      return rc;
    }

    case KEY_UP:
      if (select_index - from_row > 0)
        select_index--;
      break;

    case KEY_DOWN:
      if (select_index < results.count - 1 &&
          select_index - from_row + 1 < usable_rows())
        select_index++;
      break;

    case ' ':
      if (from_row + usable_rows() < results.count) {
        from_row += usable_rows();
      } else {
        from_row = 0;
      }
      select_index = from_row;
      print_results();
      break;

    case '\t':
      state = ST_INPUT;
      break;

    case KEY_RESIZE:
      endwin();
      clear();
      print_menu();
      move_to_line_no_blank(prompt_index);
      print_results();
      assert(select_index >= from_row);
      if (select_index - from_row + 1 > usable_rows()) {
        // the selected row was just made offscreen by a window resize
        select_index = from_row;
      }
      break;
  }

  return 0;
}

int ncurses_ui(clink_db_t *db) {

  // save database pointer
  database = db;

  // setup our initial (empty) accrued text at the prompt
  left = calloc(1, BUFSIZ);
  if (left == NULL)
    return ENOMEM;
  left_size = BUFSIZ;

  right = calloc(1, BUFSIZ);
  if (right == NULL) {
    free(left);
    return ENOMEM;
  }
  right_size = BUFSIZ;

  // Save any registered SIGTSTP handler, that we will need to temporarily
  // restore later. There is most likely no handler (SIG_DFL) at this point,
  // but future-proof this against us registering handlers elsewhere in Clink.
  (void)sigaction(SIGTSTP, NULL, &original_sigtstp_handler);

  // We also need to stash the SIGWINCH handler. See earlier in this file where
  // we open Vim for an explanation of these shenanigans.
  (void)sigaction(SIGWINCH, NULL, &original_sigwinch_handler);

  // initialise Ncurses
  (void)initscr();
  if (option.colour == ALWAYS) {
    if (has_colors())
      colour = init_ncurses_colours() == 0;
  }

  keypad(stdscr, TRUE);
  (void)cbreak();

  // these need to come after ncurses init
  x = offset_x(0);
  y = offset_y(0);
  print_menu();
  refresh();

  int rc = 0;

  for (;;) {

    switch (state) {

      case ST_INPUT:
        rc = handle_input();
        break;

      case ST_ROWSELECT:
        rc = handle_select();
        break;

      case ST_EXITING:
        goto break2;
    }

    if (rc)
      break;
  }

break2:

  endwin();

  return rc;
}
