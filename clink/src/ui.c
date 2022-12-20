#include "ui.h"
#include "../../common/compiler.h"
#include "colour.h"
#include "find_repl.h"
#include "option.h"
#include "path.h"
#include "re.h"
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

/// text the user has entered to the right of the cursor
static char *right;

/// database previously loaded
static clink_db_t *database;

/// absolute path to our accompanying `clink-repl` script
static char *clink_repl;

/// current working directory
static char *cur_dir;

/// `disppath` but assumes an absolute input, so no need to allocate
static const char *display_path(const char *path) {
  assert(path != NULL);
  assert(path[0] == '/');

  assert(cur_dir != NULL);
  if (strncmp(path, cur_dir, strlen(cur_dir)) != 0)
    return path;
  if (path[strlen(cur_dir)] != '/')
    return path;

  return &path[strlen(cur_dir) + 1];
}

typedef struct {
  clink_symbol_t *rows;
  size_t count;
  size_t size;
} results_t;

/// results of the last query we ran
static results_t results;

/// number of result columns excluding the hot key
enum { COLUMN_COUNT = 4 };

/// print something, updating the display immediately
#define PRINT(args...)                                                         \
  do {                                                                         \
    printf(args);                                                              \
    fflush(stdout);                                                            \
  } while (0)

/// print something, accounting for the possibility of stripping colours
///
/// \param str String to print
/// \param lock An optional format to keep applied
static void print_colour(const char *str, const char *lock) {
  assert(str != NULL);
  if (option.colour == ALWAYS) {
    bool in_csi = false;
    for (size_t i = 0; str[i] != '\0'; ++i) {
      putchar(str[i]);
      if (in_csi && str[i] == 'm') {
        // exiting CSI; reapply the locked format
        if (lock != NULL)
          printf("%s", lock);
      } else if (!in_csi && str[i] == '\033') {
        in_csi = true;
      }
    }
  } else {
    printf_bw(str, stdout);
  }
  fflush(stdout);
}

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
  PRINT("\033[%zu;%zuH", row, column);
}

static const char CLRTOEOL[] = "\033[K";

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

  // note to the user what we are doing
  {
    size_t rows = screen_get_rows();
    move(rows - FUNCTIONS_SZ, 1);
    PRINT("   formatting results…%s", CLRTOEOL);
    (void)spinner_on(rows - FUNCTIONS_SZ, 2);
  }

  // free any previous results
  for (size_t i = 0; i < results.count; ++i)
    clink_symbol_clear(&results.rows[i]);
  results.count = 0;

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
        // Update what we are doing. Inline the move and `CLRTOEOL` so we can do
        // it all while holding the stdout lock and avoid racing with the
        // spinner.
        size_t rows = screen_get_rows();
        PRINT("\033[%zu;4Hsyntax highlighting %s…%s", rows - FUNCTIONS_SZ,
              display_path(target->path), CLRTOEOL);

        // ignore non-fatal failure of highlighting
        (void)clink_vim_read_into(database, target->path);

        // update what we are doing
        PRINT("\033[%zu;4Hformatting results…%s", rows - FUNCTIONS_SZ,
              CLRTOEOL);
      } else if (r != EALREADY) {
        rc = r;
        break;
      }
      // also ignore failure here and continue
      (void)clink_db_get_content(database, target->path, target->lineno,
                                 &target->context);
    }
  }

done:

  spinner_off();
  {
    size_t rows = screen_get_rows();
    move(rows - FUNCTIONS_SZ, 1);
    PRINT("%s", CLRTOEOL);
  }

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
    PRINT("%s:", functions[i].prompt);
  }
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
      size_t w = i == 0   ? strlen(display_path(sym->path))
                 : i == 1 ? (sym->parent == NULL ? 0 : strlen(sym->parent))
                 : i == 2 ? digit_count(sym->lineno)
                          : (sym->context == NULL ? 0 : strlen(sym->context));
      if (w > widths[i])
        widths[i] = w;
    }
  }

  // print column headings
  move(1, 1);
  PRINT("  ");
  for (size_t i = 0; i < COLUMN_COUNT; ++i)
    PRINT("%-*s ", (int)widths[i], HEADINGS[i]);
  PRINT("%s", CLRTOEOL);

  // print the rows
  for (size_t i = 0; i < rows - FUNCTIONS_SZ - 1 - 1; ++i) {
    move(2 + i, 1);
    if (i < row_count) {
      // is this the selected row?
      bool is_selected = i + from_row == select_index;
      // blue-highlight this row if it is selected
      if (option.colour == ALWAYS && is_selected)
        PRINT("\033[44m");
      PRINT("%c ", hotkey(i));
      for (size_t j = 0; j < COLUMN_COUNT; ++j) {
        const clink_symbol_t *sym = &results.rows[i + from_row];
        switch (j) {
        case 0: { // file
          const char *display = display_path(sym->path);
          PRINT("%-*s ", (int)widths[j], display);
          break;
        }
        case 1: // function
          PRINT("%-*s ", (int)widths[j],
                sym->parent == NULL ? "" : sym->parent);
          break;
        case 2: // line
          PRINT("%*lu ", (int)widths[j], sym->lineno);
          break;
        case 3: // context
          if (sym->context != NULL)
            print_colour(sym->context, is_selected ? "\033[44m" : NULL);
          break;
        }
      }
      PRINT("\033[0m");
    }
    PRINT("%s", CLRTOEOL);
  }

  // print footer
  move(rows - FUNCTIONS_SZ, 1);
  PRINT("* ");
  if (results.count == 0) {
    PRINT("No results");
  } else {
    PRINT("Lines %zu-%zu of %zu", from_row + 1, from_row + row_count,
          results.count);
    if (from_row + row_count < results.count) {
      PRINT(", %zu more - press the space bar to display more",
            results.count - from_row - row_count);
    } else if (from_row > 0) {
      PRINT(", press the space bar to display the first lines again");
    }
  }
  PRINT(" *%s", CLRTOEOL);

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
  PRINT("%s%s", left, right);
  x += utf8_strlen(left);
}

static void move_to_line(size_t target) {

  // blank the current line
  move(y, offset_x(prompt_index));
  PRINT("%s", CLRTOEOL);

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
        return ENOMEM;
      do { // check this is a valid regex before doing the lookup
        int rc = re_check(query);
        if (rc == 0)
          break;
        free(query);
        move(screen_get_rows() - FUNCTIONS_SZ, 1);
        print_colour("  \033[31;1mERROR\033[0m: ", NULL);
        printf("%s", rc == EINVAL ? "invalid regex" : strerror(rc));
        return 0;
      } while (0);
      int rc = functions[prompt_index].handler(query);
      free(query);
      if (UNLIKELY(rc))
        return rc;
      from_row = 0;
      select_index = 0;
      if (results.count > 0) {
        state = ST_ROWSELECT;
      } else {
        // force note about lack of results
        print_results();
      }
    }
    return 0;
  }

  if (e.type == EVENT_KEYPRESS && e.value == 0x17) { // Ctrl-W
    while (strlen(left) > 0 && isspace(left[strlen(left) - 1]))
      left[strlen(left) - 1] = '\0';
    while (strlen(left) > 0 && !isspace(left[strlen(left) - 1]))
      left[strlen(left) - 1] = '\0';
    {
      char *l = realloc(left, strlen(left) + 1);
      if (UNLIKELY(l == NULL))
        return ENOMEM;
      left = l;
    }
    return 0;
  }

  if (e.type == EVENT_KEYPRESS && e.value == 0xb) { // Ctrl-K
    free(right);
    right = strdup("");
    if (UNLIKELY(right == NULL))
      return ENOMEM;
    return 0;
  }

  if (e.type == EVENT_KEYPRESS && e.value == 0x15) { // Ctrl-U
    free(left);
    left = strdup("");
    if (UNLIKELY(left == NULL))
      return ENOMEM;
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

      // expand right
      {
        char *r = realloc(right, strlen(right) + len + 1);
        if (UNLIKELY(r == NULL))
          return ENOMEM;
        right = r;
      }

      // insert the new character
      memmove(right + len, right, strlen(right) + 1);
      memcpy(right, left + strlen(left) - len, len);

      // remove it from the left side
      left[strlen(left) - len] = '\0';
      {
        char *l = realloc(left, strlen(left) + 1);
        if (UNLIKELY(l == NULL))
          return ENOMEM;
        left = l;
      }
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

      // expand left
      {
        char *l = realloc(left, strlen(left) + len + 1);
        if (UNLIKELY(l == NULL))
          return ENOMEM;
        left = l;
      }

      // insert the new character
      strncat(left, right, len);

      // remove it from the right side
      memmove(right, right + len, strlen(right) - len + 1);
      {
        char *r = realloc(right, strlen(right) + 1);
        if (UNLIKELY(r == NULL))
          return ENOMEM;
        right = r;
      }
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

  if (e.type == EVENT_KEYPRESS && (e.value == 0x1 ||         // Ctrl-A
                                   e.value == 0x7e315b1b)) { // Home

    // expand right
    {
      char *r = realloc(right, strlen(left) + strlen(right) + 1);
      if (UNLIKELY(r == NULL))
        return ENOMEM;
      right = r;
    }

    // make room for text to be added
    memmove(right + strlen(left), right, strlen(right) + 1);

    // add the text
    memcpy(right, left, strlen(left));

    // clear the text we just transferred
    free(left);
    left = strdup("");
    if (UNLIKELY(left == NULL))
      return ENOMEM;

    return 0;
  }

  if (e.type == EVENT_KEYPRESS && (e.value == 0x5 ||         // Ctrl-E
                                   e.value == 0x7e345b1b)) { // End

    // expand left
    {
      char *l = realloc(left, strlen(left) + strlen(right) + 1);
      if (UNLIKELY(l == NULL))
        return ENOMEM;
      left = l;
    }

    // append the right hand text
    strcat(left, right);

    // clear the text we just transferred
    free(right);
    right = strdup("");
    if (UNLIKELY(right == NULL))
      return ENOMEM;

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

  if (e.type == EVENT_KEYPRESS && (e.value == 0x8 ||   // Ctrl-H
                                   e.value == 0x7f)) { // Backspace
    if (strlen(left) > 0) {

      // we assume `left` contains only valid UTF-8 characters, so we can find
      // the start of the last character by scanning 0b10xxxxxx bytes
      size_t len = 1;
      while (len < strlen(left) &&
             ((uint8_t)left[strlen(left) - len] >> 6) == 0b10)
        ++len;

      left[strlen(left) - len] = '\0';
      {
        char *l = realloc(left, strlen(left) + 1);
        if (UNLIKELY(l == NULL))
          return ENOMEM;
        left = l;
      }
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
      {
        char *r = realloc(right, strlen(right) + 1);
        if (UNLIKELY(r == NULL))
          return ENOMEM;
        right = r;
      }
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

    // expand left
    {
      char *l = realloc(left, strlen(left) + len + 1);
      if (UNLIKELY(l == NULL))
        return ENOMEM;
      left = l;
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

    if (from_row + index < results.count && index < usable_rows()) {
      select_index = from_row + index;
      goto enter;
    }
    return 0;
  }

  if (e.type == EVENT_KEYPRESS && e.value == 0xa) { // Enter
  enter:
    screen_free();

    (void)clink_vim_open(results.rows[select_index].path,
                         results.rows[select_index].lineno,
                         results.rows[select_index].colno, clink_repl,
                         clink_repl == NULL ? NULL : database);

    int rc = screen_init();
    if (rc != 0)
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
        select_index - from_row + 1 < usable_rows() &&
        select_index - from_row + 1 < sizeof(HOTKEYS) - 1)
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

int ui(clink_db_t *db) {

  // save database pointer
  database = db;

  // see if we can locate `clink-repl`; failure is non-critical
  clink_repl = find_repl();

  int rc = 0;

  // setup our initial (empty) accrued text at the prompt
  left = strdup("");
  if (UNLIKELY(left == NULL)) {
    rc = ENOMEM;
    goto done;
  }

  right = strdup("");
  if (UNLIKELY(right == NULL)) {
    rc = ENOMEM;
    goto done;
  }

  if (UNLIKELY((rc = cwd(&cur_dir))))
    goto done;

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
      print_results();
      rc = handle_select();
      break;

    case ST_EXITING:
      goto done;
    }

    if (rc)
      break;
  }

done:
  for (size_t i = 0; i < results.count; ++i)
    clink_symbol_clear(&results.rows[i]);
  results.count = 0;
  free(results.rows);
  results.rows = NULL;
  results.size = 0;

  screen_free();
  free(cur_dir);
  cur_dir = NULL;
  free(right);
  right = NULL;
  free(left);
  left = NULL;
  free(clink_repl);
  clink_repl = NULL;

  return rc;
}
