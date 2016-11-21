#include <cassert>
#include <cstdlib>
#include <curses.h>
#include "Database.h"
#include <string>
#include <string.h>
#include "Symbol.h"
#include "UICurses.h"
#include "util.h"
#include <vector>
#include "Vim.h"

using namespace std;

struct ResultRow {
    vector<string> text;
    string path;
    unsigned line;
    unsigned col;
};

struct Results {
    vector<string> headings;
    vector<ResultRow> rows;
};

static Results *find_symbol(const Database &db, const char *query) {
    Results *results = new Results;

    results->headings.push_back("File");
    results->headings.push_back("Function");
    results->headings.push_back("Line");
    results->headings.push_back("");

    vector<Symbol> vs = db.find_symbol(query);
    for (auto s : vs) {
        ResultRow row {
            .text = { s.path(), s.parent(), to_string(s.line()),
                lstrip(s.context()) },
            .path = s.path(),
            .line = s.line(),
            .col = s.col(),
        };
        results->rows.push_back(row);
    }

    return results;
}

static struct {
    const char *prompt;
    Results *(*handler)(const Database &db, const char *query);
} functions[] = {
    { "Find this C symbol", find_symbol },
    { "Find this global definition", nullptr },
    { "Find functions called by this function", nullptr },
    { "Find functions calling this function", nullptr },
    { "Find this file", nullptr },
    { "Find files #including this file", nullptr },
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

static char hotkey(unsigned index) {
    switch (index) {
        case 0 ... 9: return '0' + index;
        case 10 ... 35: return 'a' + index - 10;
        case 36 ... 61: return 'A' + index - 36;
        default: return -1;
    }
}

static int print_results(const Results &results, unsigned from_row) {
    assert(from_row == 0 || from_row < results.rows.size());

    /* The number of rows we can fit is the number of lines on the screen with
     * some room extracted for the column headings, menu and status.
     */
    if ((unsigned)LINES < FUNCTIONS_SZ + 2 + 1 + 1)
        return -1;
    unsigned row_count = LINES - FUNCTIONS_SZ - 2 - 1;
    if (row_count > results.rows.size() - from_row) {
        /* Can't show more rows than we have. */
        row_count = results.rows.size() - from_row;
    }
    if (row_count > 62) {
        /* We only have 62 addressable rows. */
        row_count = 62;
    }

    /* Figure out column widths. */
    vector<unsigned> widths;
    for (unsigned i = 0; i < results.headings.size(); i++) {
        /* Find the maximum width of this column's results. */
        unsigned width = results.headings[i].size();
        for (unsigned j = from_row; j < from_row + row_count; j++) {
            assert(j < results.rows.size());
            assert(i < results.rows[j].text.size());
            if (results.rows[j].text[i].size() > width)
                width = results.rows[j].text[i].size();
        }
        widths.push_back(width + 1);
    }
    
    /* Print column headings. */
    move(0, 0);
    printw("  ");
    for (unsigned i = 0; i < results.headings.size(); i++) {
        size_t padding = widths[i] - results.headings[i].size();
        string blank(padding, ' ');
        printw("%s%s", results.headings[i].c_str(), blank.c_str());
    }
    clrtoeol();

    /* Print the rows. */
    for (unsigned i = 0; i < 61 && i < LINES - FUNCTIONS_SZ - 1 - 1; i++) {
        move(1 + i, 0);
        if (from_row + i < row_count) {
            printw("%c ", hotkey(i));
            for (unsigned j = 0; j < widths.size(); j++) {
                size_t padding = widths[j] - results.rows[i + from_row].text[j].size();
                string blank(padding, ' ');
                printw("%s%s", results.rows[i + from_row].text[j].c_str(), blank.c_str());
            }
        }
        clrtoeol();
    }

    /* Print footer. */
    move(LINES - FUNCTIONS_SZ - 1, 0);
    printw("* Lines %u-%u of %u", from_row + 1, from_row + row_count,
        results.rows.size());
    if (from_row + row_count < results.rows.size())
        printw(", %u more - press the space bar to display more",
            results.rows.size() - from_row - row_count);
    printw(" *");
    clrtoeol();

    return 0;
}

void UICurses::move_to_line(unsigned target) {
    // Blank the current line.
    move(m_y, offset_x(m_index));
    clrtoeol();

    m_index = target;
    m_x = offset_x(m_index);
    m_y = offset_y(m_index);

    // Paste the previous contents into the new line.
    move(m_y, m_x);
    printw("%s%s", m_left.c_str(), m_right.c_str());
    m_x += m_left.size();
}

void UICurses::handle_input(Database &db) {

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
                delete m_results;
                string query = m_left + m_right;
                m_results = functions[m_index].handler(db, query.c_str());
                print_results(*m_results, 0);
                m_from_row = 0;
                m_select_index = 0;
                if (!m_results->rows.empty())
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
            if (!m_results->rows.empty())
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

        case KEY_BACKSPACE:
            if (!m_left.empty()) {
                m_left.pop_back();
                m_x--;
                printw("%s", m_right.c_str());
                clrtoeol();
            }
            break;

        case KEY_DC:
            if (!m_right.empty()) {
                m_right = m_right.substr(1, m_right.size() - 1);
                printw("%s", m_right.c_str());
                clrtoeol();
            }
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
    move(m_select_index - m_from_row + 1, 0);
    int c = getch();

    switch (c) {

        case 4: /* Ctrl-D */
            m_state = UICS_EXITING;
            m_ret = EXIT_SUCCESS;
            break;

        case 10: { /* enter */
            def_prog_mode();
            endwin();
            int ret = vim_open(m_results->rows[m_select_index].path,
                m_results->rows[m_select_index].line,
                m_results->rows[m_select_index].col);
            if (ret != EXIT_SUCCESS) {
                m_state = UICS_EXITING;
                m_ret = ret;
            }
            reset_prog_mode();
            refresh();
            break;
        }

        case KEY_UP:
            if (m_select_index - m_from_row > 0)
                m_select_index--;
            break;

        case KEY_DOWN:
            if (m_select_index < m_results->rows.size() - 1 &&
                    m_select_index - m_from_row < 61)
                m_select_index++;
            break;

        case '\t':
            m_state = UICS_INPUT;
            break;
    }
}

int UICurses::run(Database &db) {

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

UICurses::UICurses() : m_state(UICS_INPUT), m_left(""), m_right(""), m_index(0),
        m_select_index(0), m_results(nullptr), m_from_row(0) {

    (void)initscr();
    keypad(stdscr, TRUE);
    (void)cbreak();

    // These need to come after ncurses init.
    m_x = offset_x(0);
    m_y = offset_y(0);
}

UICurses::~UICurses() {
    delete m_results;
    endwin();
}
