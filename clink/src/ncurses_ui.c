#include "ncurses_ui.h"
#include "../../common/compiler.h"
#include "colour.h"
#include "option.h"
#include "screen.h"
#include "set.h"
#include "spinner.h"
#include <assert.h>
#include <clink/clink.h>
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

/// will these two symbols appear identically in the results list?
static bool are_duplicates(const clink_symbol_t *a, const clink_symbol_t *b) {
  if (LIKELY(strcmp(a->name, b->name) != 0))
    return false;
  if (LIKELY(strcmp(a->path, b->path) != 0))
    return false;
  if (LIKELY(a->lineno != b->lineno))
    return false;
  if (LIKELY(a->colno != b->colno))
    return false;
  return true;
}

static void move(size_t row, size_t column) {
  printf("\033[%zu;%zuH", row, column);
  fflush(stdout);
}

static void clrtoeol(void) {
  printf("\033[K");
  fflush(stdout);
}

static int find_symbol(const char *query);
static int find_definition(const char *query);
static int find_call(const char *query);
static int find_caller(const char *query);
static int find_includer(const char *query);

static const struct searcher {
  const char *prompt;
  int (*handler)(const char *query);
} functions[] = {
    {"Find this C symbol", find_symbol},
    {"Find this definition", find_definition},
    {"Find functions called by this function", find_call},
    {"Find functions calling this function", find_caller},
    {"Find files #including this file", find_includer},
};

static const size_t FUNCTIONS_SZ = sizeof(functions) / sizeof(functions[0]);

static int format_results(clink_iter_t *it) {

  // free any previous results
  for (size_t i = 0; i < results.count; ++i)
    clink_symbol_clear(&results.rows[i]);
  free(results.rows);
  results.rows = NULL;
  results.count = results.size = 0;

  int rc = 0;

  // track which files we have attempted to syntax-highlight
  set_t *highlighted = NULL;
  if (UNLIKELY((rc = set_new(&highlighted))))
    goto done;

  while (true) {

    // retrieve the next symbol
    const clink_symbol_t *symbol = NULL;
    if ((rc = clink_iter_next_symbol(it, &symbol))) {
      if (LIKELY(rc == ENOMSG)) // exhausted iterator
        rc = 0;
      break;
    }

    // skip if the containing file has been deleted or moved
    assert(symbol->path != NULL);
    if (UNLIKELY(access(symbol->path, F_OK) < 0))
      continue;

    // If this duplicates the previous result, skip it. This can happen when,
    // e.g., there are two identically positioned records for a function call
    // (one for the reference to the function and one for the call itself) and
    // we are doing a general search for a symbol.
    if (results.count > 0) {
      const clink_symbol_t *previous = &results.rows[results.count - 1];
      if (are_duplicates(previous, symbol))
        continue;
    }

    // expand results collection if necessary
    if (results.size == results.count) {
      size_t s = results.size == 0 ? 128 : results.size * 2;
      clink_symbol_t *r = realloc(results.rows, s * sizeof(results.rows[0]));
      if (UNLIKELY(r == NULL)) {
        rc = ENOMEM;
        break;
      }
      results.rows = r;
      results.size = s;
    }

    // append the new symbol
    clink_symbol_t *target = &results.rows[results.count];
    if (UNLIKELY((rc = clink_symbol_copy(target, symbol))))
      break;
    ++results.count;

    // if the context is missing (can happen if it was delayed), highlight the
    // file now
    if (target->context == NULL) {
      // if we have not tried highlighting, try it now
      const char *path = target->path;
      int r = set_add(highlighted, &path);
      if (r == 0) {
        // note to the user what we are doing
        size_t rows = screen_get_rows();
        move(rows - FUNCTIONS_SZ, 1);
        printf("   syntax highlighting %s…", target->path);
        fflush(stdout);
        (void)spinner_on(rows - FUNCTIONS_SZ, 2);

        // ignore non-fatal failure of highlighting
        (void)clink_vim_read_into(database, target->path);

        spinner_off();
        move(rows - FUNCTIONS_SZ, 1);
        clrtoeol();
      } else if (r != EALREADY) {
        rc = r;
        break;
      }
      // also ignore failure here and continue
      (void)clink_db_get_content(database, target->path, target->lineno,
                                 &target->context);
    }

    // strip leading white space from the context for neater output
    if (target->context != NULL) {
      for (char *p = target->context; *p != '\0';) {
        if (isspace(*p)) {
          memmove(p, p + 1, strlen(p));
        } else if (*p == '\033') { // CSI
          // skip over it to the terminator
          for (char *q = p + 1; *q != '\0'; ++q) {
            if (*q == 'm') {
              p = q;
              break;
            }
          }
          ++p;
        } else { // non-white-space content
          break;
        }
      }
    }
  }

done:

  set_free(&highlighted);
  clink_iter_free(&it);

  return rc;
}

// wrappers for each database query

static int find_symbol(const char *query) {

  clink_iter_t *it = NULL;
  int rc = clink_db_find_symbol(database, query, &it);
  if (UNLIKELY(rc))
    return rc;

  return format_results(it);
}

static int find_definition(const char *query) {

  clink_iter_t *it = NULL;
  int rc = clink_db_find_definition(database, query, &it);
  if (UNLIKELY(rc))
    return rc;

  return format_results(it);
}

static int find_call(const char *query) {

  clink_iter_t *it = NULL;
  int rc = clink_db_find_call(database, query, &it);
  if (UNLIKELY(rc))
    return rc;

  return format_results(it);
}

static int find_caller(const char *query) {

  clink_iter_t *it = NULL;
  int rc = clink_db_find_caller(database, query, &it);
  if (UNLIKELY(rc))
    return rc;

  return format_results(it);
}

static int find_includer(const char *query) {

  clink_iter_t *it = NULL;
  int rc = clink_db_find_includer(database, query, &it);
  if (UNLIKELY(rc))
    return rc;

  return format_results(it);
}

static void print_menu(void) {
  for (size_t i = 0; i < FUNCTIONS_SZ; ++i) {
    move(screen_get_rows() - FUNCTIONS_SZ + 1 + i, 1);
    printf("%s:", functions[i].prompt);
  }
  fflush(stdout);
}

static size_t offset_x(size_t index) {
  assert(index < FUNCTIONS_SZ);
  return 1 + strlen(functions[index].prompt) + strlen(": ");
}

static size_t offset_y(size_t index) {
  assert(index < FUNCTIONS_SZ);
  return screen_get_rows() - FUNCTIONS_SZ + index + 1;
}

static const char HOTKEYS[] =
    "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

static char hotkey(unsigned index) {
  if (index < sizeof(HOTKEYS) - 1)
    return HOTKEYS[index];
  return -1;
}

static size_t usable_rows(void) {
  return screen_get_rows() - FUNCTIONS_SZ - 2 - 1;
}

static void pad(size_t width) {
  for (size_t i = 0; i < width; ++i)
    printf(" ");
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
  static const char *HEADINGS[COLUMN_COUNT] = {"File", "Function", "Line", ""};

  size_t rows = screen_get_rows();

  // the number of rows we can fit is the number of lines on the screen with
  // some room extracted for the column headings, menu and status
  if (rows < FUNCTIONS_SZ + 2 + 1 + 1)
    return -1;
  size_t row_count = usable_rows();
  if (row_count > results.count - from_row) {
    // cannot show more rows than we have
    row_count = results.count - from_row;
  }
  if (row_count > sizeof(HOTKEYS) - 1)
    row_count = sizeof(HOTKEYS) - 1;

  // figure out column widths
  size_t widths[COLUMN_COUNT] = {0};
  for (size_t i = 0; i < COLUMN_COUNT; ++i) {
    widths[i] = strlen(HEADINGS[i]);
    // find the maximum width of this column’s results
    for (size_t j = from_row; j < from_row + row_count; ++j) {
      assert(j < results.count);
      const clink_symbol_t *sym = &results.rows[j];
      size_t w = i == 0   ? strlen(sym->path)
                 : i == 1 ? (sym->parent == NULL ? 0 : strlen(sym->parent))
                 : i == 2 ? digit_count(sym->lineno)
                          : (sym->context == NULL ? 0 : strlen(sym->context));
      if (w > widths[i])
        widths[i] = w;
    }
  }

  // print column headings
  move(1, 1);
  printf("  ");
  for (size_t i = 0; i < COLUMN_COUNT; ++i) {
    printf("%s ", HEADINGS[i]);
    size_t padding = widths[i] - strlen(HEADINGS[i]);
    pad(padding);
  }
  clrtoeol();

  // print the rows
  for (size_t i = 0; i < rows - FUNCTIONS_SZ - 1 - 1; ++i) {
    move(2 + i, 1);
    if (i < row_count) {
      printf("%c ", hotkey(i));
      for (size_t j = 0; j < COLUMN_COUNT; ++j) {
        const clink_symbol_t *sym = &results.rows[i + from_row];
        size_t padding = widths[j] + 1;
        switch (j) {
        case 0: // file
          padding -= strlen(sym->path);
          printf("%s", sym->path);
          pad(padding);
          break;
        case 1: // function
          padding -= sym->parent == NULL ? 0 : strlen(sym->parent);
          if (sym->parent != NULL)
            printf("%s", sym->parent);
          pad(padding);
          break;
        case 2: // line
          padding -= digit_count(sym->lineno) + 1;
          pad(padding);
          printf("%lu ", sym->lineno);
          break;
        case 3: // context
          if (sym->context != NULL) {
            if (option.colour == ALWAYS) {
              printf("%s", sym->context);
            } else {
              printf_bw(sym->context, stdout);
            }
          }
          break;
        }
      }
    }
    clrtoeol();
  }

  // print footer
  move(rows - FUNCTIONS_SZ, 1);
  printf("* ");
  if (results.count == 0) {
    printf("No results");
  } else {
    printf("Lines %zu-%zu of %zu", from_row + 1, from_row + row_count,
           results.count);
    if (from_row + row_count < results.count) {
      printf(", %zu more - press the space bar to display more",
             results.count - from_row - row_count);
    } else if (from_row > 0) {
      printf(", press the space bar to display the first lines again");
    }
  }
  printf(" *");
  clrtoeol();
  fflush(stdout);

  return 0;
}

/** `strlen`, accounting for UTF-8 characters
 *
 * This function assumes the input is valid UTF-8.
 *
 * \param s String to measure
 * \return Number of UTF-8 characters in `s`
 */
static size_t utf8_strlen(const char *s) {
  assert(s != NULL);
  size_t length = 0;
  while (*s != '\0') {
    if ((((uint8_t)*s) >> 7) == 0) {
      ++length;
      ++s;
    } else if ((((uint8_t)*s) >> 5) == 0b110) {
      ++length;
      s += 2;
    } else if ((((uint8_t)*s) >> 4) == 0b1110) {
      ++length;
      s += 3;
    } else {
      assert((((uint8_t)*s) >> 3) == 0b11110);
      ++length;
      s += 4;
    }
  }
  return length;
}

static void move_to_line_no_blank(size_t target) {
  prompt_index = target;
  x = offset_x(prompt_index);
  y = offset_y(prompt_index);

  // paste the previous contents into the new line
  move(y, x);
  printf("%s%s", left, right);
  fflush(stdout);
  x += utf8_strlen(left);
}

static void move_to_line(size_t target) {

  // blank the current line
  move(y, offset_x(prompt_index));
  clrtoeol();

  move_to_line_no_blank(target);
}

static void refresh(void) {
  screen_clear();
  print_menu();
  if (results.count > 0)
    print_results();
}

/// how many valid printable UTF-8 bytes in this key pressed?
static size_t utf8_charlen(uint32_t key) {
  if (key <= 127) {
    if (iscntrl((int)key))
      return 0;
    return 1;
  }
  const uint8_t bytes[] = {key & 0xff, (key >> 8) & 0xff, (key >> 16) & 0xff,
                           key >> 24};
  if (bytes[1] >> 6 != 0b10)
    return 0;
  if (bytes[0] >> 5 == 0b110)
    return 2;
  if (bytes[2] >> 6 != 0b10)
    return 0;
  if (bytes[0] >> 4 == 0b1110)
    return 3;
  if (bytes[3] >> 6 != 0b10)
    return 0;
  if (bytes[0] >> 3 == 0b11110)
    return 4;
  return 0;
}

static int handle_input(void) {

  move_to_line(prompt_index);
  move(y, x);
  event_t e = screen_read();

  if (e.type == EVENT_KEYPRESS && e.value == 0x4) { // Ctrl-D
    state = ST_EXITING;
    return 0;
  }

  if (e.type == EVENT_SIGNAL && e.value == SIGINT) {
    state = ST_EXITING;
    return 0;
  }

  if (e.type == EVENT_KEYPRESS && e.value == 0xa) { // Enter
    if (strlen(left) > 0 || strlen(right) > 0) {
      char *query = NULL;
      if (UNLIKELY(asprintf(&query, "%s%s", left, right) < 0))
        return errno;
      int rc = functions[prompt_index].handler(query);
      if (UNLIKELY(rc))
        return rc;
      from_row = 0;
      select_index = 0;
      print_results();
      if (results.count > 0)
        state = ST_ROWSELECT;
    }
    return 0;
  }

  if (e.type == EVENT_KEYPRESS && e.value == 0x17) { // Ctrl-W
    while (strlen(left) > 0 && isspace(left[strlen(left) - 1]))
      left[strlen(left) - 1] = '\0';
    while (strlen(left) > 0 && !isspace(left[strlen(left) - 1]))
      left[strlen(left) - 1] = '\0';
    return 0;
  }

  if (e.type == EVENT_KEYPRESS && e.value == 0xb) { // Ctrl-K
    right[0] = '\0';
    return 0;
  }

  if (e.type == EVENT_KEYPRESS && e.value == '\t') {
    if (results.count > 0)
      state = ST_ROWSELECT;
    return 0;
  }

  if (e.type == EVENT_KEYPRESS && e.value == 0x445b1b) { // ←
    if (strlen(left) > 0) {

      // we assume `left` contains only valid UTF-8 characters, so we can find
      // the start of the last character by scanning 0b10xxxxxx bytes
      size_t len = 1;
      while (len < strlen(left) &&
             ((uint8_t)left[strlen(left) - len] >> 6) == 0b10)
        ++len;

      // expand right if necessary
      if (strlen(right) + len >= right_size) {
        char *r = realloc(right, right_size * 2);
        if (UNLIKELY(r == NULL))
          return ENOMEM;
        right = r;
        right_size *= 2;
      }

      // insert the new character
      memmove(right + len, right, strlen(right) + 1);
      memcpy(right, left + strlen(left) - len, len);

      // remove it from the left side
      left[strlen(left) - len] = '\0';
    }
    return 0;
  }

  if (e.type == EVENT_KEYPRESS && e.value == 0x435b1b) { // →
    if (strlen(right) > 0) {

      // we assume `right` contains only valid UTF-8 characters, so we can find
      // the length of the first character by looking at its high bits
      size_t len = 1;
      if (((uint8_t)*right >> 5) == 0b110) {
        assert(strlen(right) >= 2 && "corrupted UTF-8 accrued in `right`");
        len = 2;
      } else if (((uint8_t)*right >> 4) == 0b1110) {
        assert(strlen(right) >= 3 && "corrupted UTF-8 accrued in `right`");
        len = 3;
      } else if (((uint8_t)*right >> 3) == 0b11110) {
        assert(strlen(right) >= 4 && "corrupted UTF-8 accrued in `right`");
        len = 4;
      }

      // expand left if necessary
      if (strlen(left) + len >= left_size) {
        char *l = realloc(left, left_size * 2);
        if (UNLIKELY(l == NULL))
          return ENOMEM;
        left = l;
        left_size *= 2;
      }

      // insert the new character
      strncat(left, right, len);

      // remove it from the right side
      memmove(right, right + len, strlen(right) - len + 1);
    }
    return 0;
  }

  if (e.type == EVENT_KEYPRESS && e.value == 0x415b1b) { // ↑
    if (prompt_index > 0)
      move_to_line(prompt_index - 1);
    return 0;
  }

  if (e.type == EVENT_KEYPRESS && e.value == 0x425b1b) { // ↓
    if (prompt_index < FUNCTIONS_SZ - 1)
      move_to_line(prompt_index + 1);
    return 0;
  }

  if (e.type == EVENT_KEYPRESS && e.value == 0x7e315b1b) { // Home

    // expand right if necessary
    while (strlen(left) + strlen(right) >= right_size) {
      char *r = realloc(right, right_size * 2);
      if (UNLIKELY(r == NULL))
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

    return 0;
  }

  if (e.type == EVENT_KEYPRESS && e.value == 0x7e345b1b) { // End

    // expand left if necessary
    while (strlen(left) + strlen(right) >= left_size) {
      char *l = realloc(left, left_size * 2);
      if (UNLIKELY(l == NULL))
        return ENOMEM;
      left = l;
      left_size *= 2;
    }

    // append the right hand text
    strcat(left, right);

    // clear the text we just transferred
    right[0] = '\0';

    return 0;
  }

  if (e.type == EVENT_KEYPRESS && e.value == 0x7e355b1b) { // Page Up
    move_to_line(0);
    return 0;
  }

  if (e.type == EVENT_KEYPRESS && e.value == 0x7e365b1b) { // Page Down
    move_to_line(FUNCTIONS_SZ - 1);
    return 0;
  }

  if (e.type == EVENT_KEYPRESS && e.value == 0x7f) { // Backspace
    if (strlen(left) > 0) {

      // we assume `left` contains only valid UTF-8 characters, so we can find
      // the start of the last character by scanning 0b10xxxxxx bytes
      size_t len = 1;
      while (len < strlen(left) &&
             ((uint8_t)left[strlen(left) - len] >> 6) == 0b10)
        ++len;

      left[strlen(left) - len] = '\0';
    }
    return 0;
  }

  if (e.type == EVENT_KEYPRESS && e.value == 0x7e335b1b) { // Delete
    if (strlen(right) > 0) {

      // we assume `right` contains only valid UTF-8 characters, so we can find
      // the length of the first character by looking at its high bits
      size_t len = 1;
      if (((uint8_t)*right >> 5) == 0b110) {
        assert(strlen(right) >= 2 && "corrupted UTF-8 accrued in `right`");
        len = 2;
      } else if (((uint8_t)*right >> 4) == 0b1110) {
        assert(strlen(right) >= 3 && "corrupted UTF-8 accrued in `right`");
        len = 3;
      } else if (((uint8_t)*right >> 3) == 0b11110) {
        assert(strlen(right) >= 4 && "corrupted UTF-8 accrued in `right`");
        len = 4;
      }

      memmove(right, right + len, strlen(right) - len + 1);
    }
    return 0;
  }

  if (e.type == EVENT_SIGNAL && e.value == SIGWINCH) {
    refresh();
    return 0;
  }

  if (e.type == EVENT_SIGNAL && e.value == SIGTSTP) {
    screen_free();

    // re-signal ourself now the `SIGTSTP` handler is reset
    (void)kill(0, SIGTSTP);

    int rc = screen_init();
    if (UNLIKELY(rc != 0))
      return rc;

    refresh();
    return 0;
  }

  if (e.type == EVENT_KEYPRESS) {
    size_t len = utf8_charlen(e.value);

    // expand left if necessary
    if (strlen(left) + len >= left_size) {
      char *l = realloc(left, left_size * 2);
      if (UNLIKELY(l == NULL))
        return ENOMEM;
      left = l;
      left_size *= 2;
    }

    // append the new character
    size_t end = strlen(left);
    const char bytes[] = {e.value & 0xff, (e.value >> 8) & 0xff,
                          (e.value >> 16) & 0xff, e.value >> 24};
    memcpy(left + end, bytes, len);
    left[end + len] = '\0';

    return 0;
  }

  return 0;
}

/// `isalnum` that ignores locale
static bool my_isalnum(uint32_t value) {
  if (value > 127)
    return false;
  if (isdigit((int)value))
    return true;
  if (value >= 'a' && value <= 'z')
    return true;
  if (value >= 'A' && value <= 'Z')
    return true;
  return false;
}

static int handle_select(void) {
  assert(state == ST_ROWSELECT);

  assert(select_index >= from_row);
  if (select_index - from_row + 1 > usable_rows()) {
    // The selected row is out of visible range. This can happen if the terminal
    // window resized while we were not in select mode.
    select_index = from_row;
  }
  move(select_index - from_row + 2, 1);
  event_t e = screen_read();

  if (e.type == EVENT_KEYPRESS && e.value == 0x04) { // Ctrl-D
    state = ST_EXITING;
    return 0;
  }

  if (e.type == EVENT_SIGNAL && e.value == SIGINT) {
    state = ST_EXITING;
    return 0;
  }

  if (e.type == EVENT_KEYPRESS && my_isalnum(e.value)) {

    size_t index;
    if (isdigit((int)e.value)) {
      index = (size_t)e.value - '0';
    } else if (e.value >= 'a' && e.value <= 'z') {
      index = 10 + (size_t)e.value - 'a';
    } else {
      assert(e.value >= 'A' && e.value <= 'Z');
      index = 10 + 26 + (size_t)e.value - 'A';
    }

    if (from_row + index < results.count) {
      select_index = from_row + index;
      goto enter;
    }
    return 0;
  }

  if (e.type == EVENT_KEYPRESS && e.value == 0xa) { // Enter
  enter:
    screen_free();

    int rc = clink_vim_open(results.rows[select_index].path,
                            results.rows[select_index].lineno,
                            results.rows[select_index].colno);
    if (rc != 0)
      return rc;

    if ((rc = screen_init()))
      return rc;

    refresh();
    return 0;
  }

  if (e.type == EVENT_KEYPRESS && e.value == 0x415b1b) { // ↑
    if (select_index - from_row > 0)
      select_index--;
    return 0;
  }

  if (e.type == EVENT_KEYPRESS && e.value == 0x425b1b) { // ↓
    if (select_index < results.count - 1 &&
        select_index - from_row + 1 < usable_rows())
      select_index++;
    return 0;
  }

  if (e.type == EVENT_KEYPRESS && e.value == ' ') {
    if (from_row + usable_rows() < results.count) {
      from_row += usable_rows();
    } else {
      from_row = 0;
    }
    select_index = from_row;
    print_results();
    return 0;
  }

  if (e.type == EVENT_KEYPRESS && e.value == '\t') {
    state = ST_INPUT;
    return 0;
  }

  if (e.type == EVENT_SIGNAL && e.value == SIGWINCH) {
    refresh();
    assert(select_index >= from_row);
    if (select_index - from_row + 1 > usable_rows()) {
      // the selected row was just made offscreen by a window resize
      select_index = from_row;
    }
    return 0;
  }

  if (e.type == EVENT_SIGNAL && e.value == SIGTSTP) {
    screen_free();

    // re-signal ourself now the `SIGTSTP` handler is reset
    (void)kill(0, SIGTSTP);

    int rc = screen_init();
    if (UNLIKELY(rc != 0))
      return rc;

    refresh();
    return 0;
  }

  return 0;
}

int ncurses_ui(clink_db_t *db) {

  // save database pointer
  database = db;

  int rc = 0;

  // setup our initial (empty) accrued text at the prompt
  left = calloc(1, BUFSIZ);
  if (UNLIKELY(left == NULL)) {
    rc = ENOMEM;
    goto done;
  }
  left_size = BUFSIZ;

  right = calloc(1, BUFSIZ);
  if (UNLIKELY(right == NULL)) {
    rc = ENOMEM;
    goto done;
  }
  right_size = BUFSIZ;

  // initialise screen
  if ((rc = screen_init()))
    goto done;

  x = 1;
  y = 1;
  refresh();

  while (true) {

    switch (state) {

    case ST_INPUT:
      rc = handle_input();
      break;

    case ST_ROWSELECT:
      rc = handle_select();
      break;

    case ST_EXITING:
      goto done;
    }

    if (rc)
      break;
  }

done:
  screen_free();
  free(right);
  right = NULL;
  free(left);
  left = NULL;

  return rc;
}
