#include <cassert>
#include <clink/clink.h>
#include "colours.h"
#include <cstdlib>
#include <cstring>
#include <curses.h>
#include <signal.h>
#include <string>
#include "UICurses.h"
#include "util.h"
#include <vector>

static std::vector<ResultRow> format_results(const std::vector<clink::Result> &vs) {
  std::vector<ResultRow> results;

  for (const auto &s : vs) {
    ResultRow row {
      .text = { s.symbol.path, s.symbol.parent, std::to_string(s.symbol.lineno),
                lstrip(s.context) },
      .path = s.symbol.path,
      .line = s.symbol.lineno,
      .col = s.symbol.colno,
    };
    results.push_back(row);
  }

  return results;
}

// Wrappers for each database query follow.

static std::vector<ResultRow> find_symbol(clink::Database &db, const std::string &query) {
  std::vector<clink::Result> vs = db.find_symbols(query);
  return format_results(vs);
}

static std::vector<ResultRow> find_definition(clink::Database &db, const std::string &query) {
  std::vector<clink::Result> vs = db.find_definitions(query);
  return format_results(vs);
}

static std::vector<ResultRow> find_call(clink::Database &db, const std::string &query) {
  std::vector<clink::Result> vs = db.find_calls(query);
  return format_results(vs);
}

static std::vector<ResultRow> find_caller(clink::Database &db, const std::string &query) {
  std::vector<clink::Result> vs = db.find_callers(query);
  return format_results(vs);
}

static std::vector<ResultRow> find_includer(clink::Database &db, const std::string &query) {
  std::vector<clink::Result> vs = db.find_includers(query);
  return format_results(vs);
}

static struct {
  const char *prompt;
  std::vector<ResultRow> (*handler)(clink::Database &db, const std::string &query);
} functions[] = {
  { "Find this C symbol", find_symbol },
  { "Find this definition", find_definition },
  { "Find functions called by this function", find_call },
  { "Find functions calling this function", find_caller },
  { "Find files #including this file", find_includer },
};

static const size_t FUNCTIONS_SZ = sizeof(functions) / sizeof(functions[0]);

static void print_menu() {
  move(LINES - FUNCTIONS_SZ, 0);
  for (unsigned i = 0; i < FUNCTIONS_SZ; i++)
    printw("%s: \n", functions[i].prompt);
}

static unsigned offset_x(unsigned index) {
  assert(index < FUNCTIONS_SZ);
  return strlen(functions[index].prompt) + sizeof(": ") - 1;
}

static unsigned offset_y(unsigned index) {
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

static unsigned usable_rows() {
  return LINES - FUNCTIONS_SZ - 2 - 1;
}

static int print_results(const std::vector<ResultRow> &results,
    unsigned from_row, bool colour) {
  assert(from_row == 0 || from_row < results.size());

  // The column headings, excluding the initial hotkey column.
  static const std::string HEADINGS[] = { "File", "Function", "Line", "" };

  /* The number of rows we can fit is the number of lines on the screen with
   * some room extracted for the column headings, menu and status.
   */
  if ((unsigned)LINES < FUNCTIONS_SZ + 2 + 1 + 1)
    return -1;
  unsigned row_count = usable_rows();
  if (row_count > results.size() - from_row) {
    /* Can't show more rows than we have. */
    row_count = results.size() - from_row;
  }
  if (row_count > sizeof(HOTKEYS) - 1) {
    row_count = sizeof(HOTKEYS) - 1;
  }

  /* Figure out column widths. */
  std::vector<unsigned> widths;
  {
    size_t i = 0;
    for (const std::string &heading : HEADINGS) {
      /* Find the maximum width of this column's results. */
      unsigned width = heading.size();
      for (unsigned j = from_row; j < from_row + row_count; j++) {
        assert(j < results.size());
        assert(i < results[j].text.size());
        if (results[j].text[i].size() > width)
          width = results[j].text[i].size();
      }
      widths.push_back(width + 1);
      ++i;
    }
  }

  /* Print column headings. */
  move(0, 0);
  printw("  ");
  {
    size_t i = 0;
    for (const std::string &heading : HEADINGS) {
      size_t padding = widths[i] - heading.size();
      std::string blank(padding, ' ');
      printw("%s%s", heading.c_str(), blank.c_str());
      ++i;
    }
  }
  clrtoeol();

  /* Print the rows. */
  for (unsigned i = 0; i < LINES - FUNCTIONS_SZ - 1 - 1; i++) {
    move(1 + i, 0);
    if (i < row_count) {
      printw("%c ", hotkey(i));
      for (unsigned j = 0; j < widths.size(); j++) {
        size_t padding = widths[j] - results[i + from_row].text[j].size();
        // XXX: right-align line numbers
        if (HEADINGS[j] == "Line") {
          std::string blank(padding - 1, ' ');
          printw("%s%s ", blank.c_str(), results[i + from_row].text[j].c_str());
        } else {
          if (colour) {
            printw_in_colour(results[i + from_row].text[j]);
          } else {
            printw("%s", strip_ansi(results[i + from_row].text[j]).c_str());
          }
          std::string blank(padding, ' ');
          printw("%s", blank.c_str());
        }
      }
    }
    clrtoeol();
  }

  /* Print footer. */
  move(LINES - FUNCTIONS_SZ - 1, 0);
  printw("* ");
  if (results.empty()) {
    printw("No results");
  } else {
    printw("Lines %u-%u of %u", from_row + 1, from_row + row_count,
      results.size());
    if (from_row + row_count < results.size())
      printw(", %u more - press the space bar to display more",
        results.size() - from_row - row_count);
    else if (from_row > 0)
      printw(", press the space bar to display the first lines again");
  }
  printw(" *");
  clrtoeol();

  return 0;
}

void UICurses::move_to_line_no_blank(unsigned target) {
  m_index = target;
  m_x = offset_x(m_index);
  m_y = offset_y(m_index);

  // Paste the previous contents into the new line.
  move(m_y, m_x);
  printw("%s%s", m_left.c_str(), m_right.c_str());
  m_x += m_left.size();
}

void UICurses::move_to_line(unsigned target) {
  // Blank the current line.
  move(m_y, offset_x(m_index));
  clrtoeol();

  move_to_line_no_blank(target);
}

void UICurses::handle_input(clink::Database &db) {

  echo();

  move(m_y, m_x);
  int c = getch();

  switch (c) {
    case 4: /* Ctrl-D */
      m_state = UICS_EXITING;
      m_ret = EXIT_SUCCESS;
      break;

    case 10: /* enter */
      if (!m_left.empty() || !m_right.empty()) {
        std::string query = m_left + m_right;
        m_results = functions[m_index].handler(db, query);
        print_results(m_results, 0, m_color);
        m_from_row = 0;
        m_select_index = 0;
        if (!m_results.empty())
          m_state = UICS_ROWSELECT;
      }
      break;

    case 23: { /* Ctrl-W */
      while (!m_left.empty() && isspace(m_left[m_left.size() - 1]))
        m_left.pop_back();
      while (!m_left.empty() && !isspace(m_left[m_left.size() - 1]))
        m_left.pop_back();
      move(m_y, offset_x(m_index));
      printw("%s%s", m_left.c_str(), m_right.c_str());
      clrtoeol();
      m_x = offset_x(m_index) + m_left.size();
      move(m_y, m_x);
      break;
    }

    case '\t':
      if (!m_results.empty())
        m_state = UICS_ROWSELECT;
      break;

    case KEY_LEFT:
      if (!m_left.empty()) {
        m_right = m_left.substr(m_left.size() - 1, 1) + m_right;
        m_left.pop_back();
        m_x--;
      }
      break;

    case KEY_RIGHT:
      if (!m_right.empty()) {
        m_left.push_back(m_right[0]);
        m_right = m_right.substr(1, m_right.size() - 1);
        m_x++;
      }
      break;

    case KEY_UP:
      if (m_index > 0)
        move_to_line(m_index - 1);
      break;

    case KEY_DOWN:
      if (m_index < FUNCTIONS_SZ - 1)
        move_to_line(m_index + 1);
      break;

    case KEY_HOME:
      m_right = m_left + m_right;
      m_left = "";
      m_x = offset_x(m_index);
      break;

    case KEY_END:
      m_left += m_right;
      m_right = "";
      m_x = offset_x(m_index) + m_left.size();
      break;

    case KEY_PPAGE:
      move_to_line(0);
      break;

    case KEY_NPAGE:
      move_to_line(FUNCTIONS_SZ - 1);
      break;

    case 127: // Backspace on macOS
    case KEY_BACKSPACE:
      if (!m_left.empty()) {
        m_left.pop_back();
        m_x--;
      }
      move(m_y, m_x);
      printw("%s", m_right.c_str());
      clrtoeol();
      break;

    case KEY_DC:
      if (!m_right.empty()) {
        m_right = m_right.substr(1, m_right.size() - 1);
        printw("%s", m_right.c_str());
        clrtoeol();
      }
      break;

    case KEY_RESIZE:
      endwin();
      clear();
      print_menu();
      if (!m_results.empty())
        print_results(m_results, m_from_row, m_color);
      move_to_line_no_blank(m_index);
      break;

    default:
      m_x++;
      m_left += c;
      if (!m_right.empty()) {
        printw("%s", m_right.c_str());
      }
  }
}

void UICurses::handle_select() {
  assert(m_state == UICS_ROWSELECT);

  noecho();

  assert(m_select_index >= m_from_row);
  if (m_select_index - m_from_row + 1 > usable_rows()) {
    /* The selected row is out of visible range. This can happen if the terminal
     * window resized while we were not in select mode.
     */
    m_select_index = m_from_row;
  }
  move(m_select_index - m_from_row + 1, 0);
  int c = getch();

  switch (c) {

    case 4: /* Ctrl-D */
      m_state = UICS_EXITING;
      m_ret = EXIT_SUCCESS;
      break;

/* XXX: egregious abuse of interstice to provide a parameter for function-like
 * label hotkey_select below. At least we can use scoping to contain the damage.
 */
{
unreachable();
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
      if (m_from_row + c - base < m_results.size()) {
        m_select_index = m_from_row + c - base;
        goto enter;
      }
      break;
}

    case 10: { /* enter */
enter:
      def_prog_mode();
      endwin();

      /* Restore the SIGTSTP handler we had prior to starting up ncurses.
       * Ncurses registers a SIGTSTP handler that it leaves in place until
       * process exit. This means that when we exec Vim below, if the user
       * suspends Vim (^Z) the SIGTSTP to Clink is masked by the ncurses
       * handler. A consequence of this is that the Clink menu is not
       * repainted when Vim exits. By restoring the original handler, we
       * can claw our way back to regular TTY behaviour.
       */
      struct sigaction curses_tstp;
      int read_tstp = sigaction(SIGTSTP, &m_original_sigtstp_handler,
        &curses_tstp);

      /* Blasted ncurses does the same thing with the SIGWINCH handler. As a
       * result, resizing the terminal window while Vim is open causes ncurses
       * to take incorrect actions. Vim notices the window has resized but that
       * it is missing a SIGWINCH, realises someone else is driving the boat and
       * freaks out and quits.
       */
      struct sigaction curses_winch;
      int read_winch = sigaction(SIGWINCH, &m_original_sigwinch_handler,
        &curses_winch);

      int ret = clink::vim_open(m_results[m_select_index].path,
          m_results[m_select_index].line,
          m_results[m_select_index].col);
      if (ret != EXIT_SUCCESS) {
          m_state = UICS_EXITING;
          m_ret = ret;
      }

      reset_prog_mode();

      // Restore ncurses' handlers.
      if (read_tstp == 0)
        (void)sigaction(SIGTSTP, &curses_tstp, nullptr);
      if (read_winch == 0)
        (void)sigaction(SIGWINCH, &curses_winch, nullptr);

      refresh();
      break;
    }

    case KEY_UP:
      if (m_select_index - m_from_row > 0)
        m_select_index--;
      break;

    case KEY_DOWN:
      if (m_select_index < m_results.size() - 1 &&
            m_select_index - m_from_row + 1 < usable_rows())
        m_select_index++;
      break;

    case ' ':
      if (m_from_row + usable_rows() < m_results.size())
        m_from_row += usable_rows();
      else
        m_from_row = 0;
      m_select_index = m_from_row;
      print_results(m_results, m_from_row, m_color);
      break;

    case '\t':
      m_state = UICS_INPUT;
      break;

    case KEY_RESIZE:
      endwin();
      clear();
      print_menu();
      move_to_line_no_blank(m_index);
      print_results(m_results, m_from_row, m_color);
      assert(m_select_index >= m_from_row);
      if (m_select_index - m_from_row + 1 > usable_rows()) {
        // The selected row was just made offscreen by a window resize.
        m_select_index = m_from_row;
      }
      break;
  }
}

int UICurses::run(clink::Database &db) {

  print_menu();
  refresh();

  for (;;) {

    switch (m_state) {

      case UICS_INPUT:
        handle_input(db);
        break;

      case UICS_ROWSELECT:
        handle_select();
        break;

      case UICS_EXITING:
        goto break2;
    }
  }

break2:

  return m_ret;
}

UICurses::UICurses() {

  /* Save any registered SIGTSTP handler, that we'll need to temporarily
   * restore later. There is most likely no handler (SIG_DFL) at this point,
   * but future-proof this against us registering handlers elsewhere in Clink.
   */
  (void)sigaction(SIGTSTP, nullptr, &m_original_sigtstp_handler);

  /* We also need to stash the SIGWINCH handler. See earlier in this file where
   * we open Vim for an explanation of these shenanigans.
   */
  (void)sigaction(SIGWINCH, nullptr, &m_original_sigwinch_handler);

  (void)initscr();
  m_color = has_colors();
  if (m_color) {
    if (init_ncurses_colours() != 0)
      m_color = false;
  }

  keypad(stdscr, TRUE);
  (void)cbreak();

  // These need to come after ncurses init.
  m_x = offset_x(0);
  m_y = offset_y(0);
}

UICurses::~UICurses() {
  endwin();
}
