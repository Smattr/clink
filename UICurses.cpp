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

static Results find_symbol(const Database &db, const char *query) {
    Results results;

    results.headings.push_back("File");
    results.headings.push_back("Function");
    results.headings.push_back("Line");
    results.headings.push_back("");

    vector<Symbol> vs = db.find_symbol(query);
    for (auto s : vs) {
        ResultRow row;
        row.text.push_back(s.path());
        row.text.push_back(s.parent());
        row.text.push_back(to_string(s.line()));
        row.text.push_back(lstrip(s.context()));
        row.path = s.path();
        row.line = s.line();
        row.col = s.col();
        results.rows.push_back(row);
    }

    return results;
}

static struct {
    const char *prompt;
    Results (*handler)(const Database &db, const char *query);
} functions[] = {
    { "Find this C symbol", find_symbol },
    { "Find this global definition", nullptr },
    { "Find functions called by this function", nullptr },
    { "Find functions calling this function", nullptr },
    { "Find this file", nullptr },
    { "Find files #including this file", nullptr },
};

static const size_t functions_sz = sizeof(functions) / sizeof(functions[0]);

static void print_menu() {
    move(LINES - functions_sz, 0);
    for (unsigned i = 0; i < functions_sz; i++)
        printw("%s: \n", functions[i].prompt);
}

static unsigned offset_x(unsigned index) {
    assert(index < functions_sz);
    return strlen(functions[index].prompt) + sizeof(": ") - 1;
}

static unsigned offset_y(unsigned index) {
    assert(index < functions_sz);
    return LINES - functions_sz + index;
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
    if ((unsigned)LINES < functions_sz + 2 + 1 + 1)
        return -1;
    unsigned row_count = LINES - functions_sz - 2 - 1;
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
        printw("%s%s", results.headings[i].c_str(),
            string(padding, ' ').c_str());
    }

    /* Print the rows. */
    for (unsigned i = from_row; i < from_row + row_count; i++) {
        move(1 + i - from_row, 0);
        printw("%c ", hotkey(i));
        for (unsigned j = 0; j < widths.size(); j++) {
            size_t padding = widths[j] - results.rows[i].text[j].size();
            printw("%s%s", results.rows[i].text[j].c_str(),
                string(padding, ' ').c_str());
        }
    }

    /* Print footer. */
    move(LINES - functions_sz - 1, 0);
    printw("* Lines %u-%u of %u", from_row, from_row + row_count,
        results.rows.size());
    if (from_row + row_count < results.rows.size())
        printw(", %u more - press the space bar to display more",
            results.rows.size() - from_row - row_count);
    printw(" *");

    return 0;
}

typedef enum {
    INPUT,
    ROWSELECT,
    EXITING,
} state_t;

struct State {
    state_t state;
    string left, right;
    unsigned index, x, y, select_index;
    int ret;
};

static void move_to_line(unsigned target, State &st) {
    // Blank the current line.
    move(st.y, offset_x(st.index));
    printw("%s", string(st.left.size() + st.right.size(), ' ').c_str());

    st.index = target;
    st.x = offset_x(st.index);
    st.y = offset_y(st.index);

    // Paste the previous contents into the new line.
    move(st.y, st.x);
    printw("%s%s", st.left.c_str(), st.right.c_str());
    st.x += st.left.size();
}

static void input_loop(State &st, Database &db) {

    echo();

    for (;;) {

        move(st.y, st.x);
        int c = getch();

        switch (c) {
            case 4: /* Ctrl-D */
                st.state = EXITING;
                assert(st.ret == EXIT_SUCCESS);
                return;

            case 10: /* enter */
                if (!st.left.empty() || !st.right.empty()) {
                    string query = st.left + st.right;
                    Results results = functions[st.index].handler(db, query.c_str());
                    print_results(results, 0);
                }
                break;

            case '\t':
                st.state = ROWSELECT;
                return;

            case KEY_LEFT:
                if (!st.left.empty()) {
                    st.right = st.left.substr(st.left.size() - 1, 1) + st.right;
                    st.left.pop_back();
                    st.x--;
                }
                break;

            case KEY_RIGHT:
                if (!st.right.empty()) {
                    st.left.push_back(st.right[0]);
                    st.right = st.right.substr(1, st.right.size() - 1);
                    st.x++;
                }
                break;

            case KEY_UP:
                if (st.index > 0)
                    move_to_line(st.index - 1, st);
                break;

            case KEY_DOWN:
                if (st.index < functions_sz - 1)
                    move_to_line(st.index + 1, st);
                break;

            case KEY_HOME:
                st.right = st.left + st.right;
                st.left = "";
                st.x = offset_x(st.index);
                break;

            case KEY_END:
                st.left += st.right;
                st.right = "";
                st.x = offset_x(st.index) + st.left.size();
                break;

            case KEY_PPAGE:
                move_to_line(0, st);
                break;

            case KEY_NPAGE:
                move_to_line(functions_sz - 1, st);
                break;

            case KEY_BACKSPACE:
                if (!st.left.empty()) {
                    st.left.pop_back();
                    st.x--;
                    printw("%s ", st.right.c_str());
                }
                break;

            case KEY_DC:
                if (!st.right.empty()) {
                    st.right = st.right.substr(1, st.right.size() - 1);
                    printw("%s ", st.right.c_str());
                }
                break;

            default:
                st.x++;
                st.left += c;
                if (!st.right.empty()) {
                    printw("%s", st.right.c_str());
                }
        }

    }
}

static void select_loop(State &st) {
    assert(st.state == ROWSELECT);

    noecho();

    for (;;) {

        move(st.select_index + 1, 0);
        int c = getch();

        switch (c) {

            case KEY_UP:
                st.select_index--;
                break;

            case KEY_DOWN:
                st.select_index++;
                break;

            case '\t':
                st.state = INPUT;
                return;
        }
    }
}

int UICurses::run(Database &db) {

    (void)initscr();
    keypad(stdscr, TRUE);
    (void)cbreak();

    print_menu();
    refresh();

    State st { INPUT, "", "", 0, offset_x(0), offset_y(0), 0, EXIT_SUCCESS };

    for (;;) {

        switch (st.state) {

            case INPUT:
                input_loop(st, db);
                break;

            case ROWSELECT:
                select_loop(st);
                break;

            case EXITING:
                goto break2;
        }
    }

break2:
    endwin();

    return st.ret;
}
