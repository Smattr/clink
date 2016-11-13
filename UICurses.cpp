#include <cassert>
#include <cstdlib>
#include <curses.h>
#include "Database.h"
#include <string>
#include <string.h>
#include "UICurses.h"
#include <vector>

using namespace std;

static struct {
    const char *prompt;
    void (*handler)(const char *query);
} functions[] = {
    { "Find this C symbol", nullptr },
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
    for (unsigned i = 0; i < results.headings.size(); i++) {
        size_t padding = widths[i] - results.headings[i].size();
        printw("%s%s", results.headings[i].c_str(),
            string(padding, ' ').c_str());
    }

    /* Print the rows. */
    for (unsigned i = from_row; i < from_row + row_count; i++) {
        move(i - from_row, 0);
        for (unsigned j = 0; j < widths.size(); j++) {
            size_t padding = widths[i] - results.rows[i].text[j].size();
            printw("%s%s", results.rows[i].text[j].c_str(),
                string(padding, ' ').c_str());
        }
    }

    /* Print footer. */
    move(row_count + 1, 0);
    printw("* Lines %u-%u of %u *", from_row, from_row + row_count,
        results.rows.size());

    return 0;
}

int UICurses::run(Database &db) {

    (void)initscr();
    keypad(stdscr, TRUE);
    (void)cbreak();

    print_menu();
    refresh();

    unsigned index = 0;
    unsigned x = offset_x(index);
    unsigned y = offset_y(index);
    move(y, x);

    string left, right;

    for (;;) {

        int c = getch();

        switch (c) {
            case 4: /* Ctrl-D */
                goto break2;

            case KEY_LEFT:
                if (!left.empty()) {
                    right = left.substr(left.size() - 1, 1) + right;
                    left.pop_back();
                    x--;
                    move(y, x);
                }
                break;

            case KEY_RIGHT:
                if (right.empty()) {
                    move(y, x);
                } else {
                    left.push_back(right[0]);
                    right = right.substr(1, right.size() - 1);
                    x++;
                    move(y, x);
                }
                break;

            case KEY_UP:
                if (index > 0) {
                    // Blank the current line.
                    move(y, offset_x(index));
                    printw("%s", string(left.size() + right.size(), ' ').c_str());

                    index--;
                    x = offset_x(index);
                    y = offset_y(index);

                    // Paste the previous contents into the new line.
                    move(y, x);
                    printw("%s%s", left.c_str(), right.c_str());
                    x += left.size();
                    move(y, x);
                }
                break;

            case KEY_DOWN:
                if (index < functions_sz - 1) {
                    // Blank the current line.
                    move(y, offset_x(index));
                    printw("%s", string(left.size() + right.size(), ' ').c_str());

                    index++;
                    x = offset_x(index);
                    y = offset_y(index);

                    // Paste the previous contents into the new line.
                    move(y, x);
                    printw("%s%s", left.c_str(), right.c_str());
                    x += left.size();
                    move(y, x);
                }
                break;

            case KEY_HOME:
                right = left + right;
                left = "";
                x = offset_x(index);
                move(y, x);
                break;

            case KEY_END:
                left += right;
                right = "";
                x = offset_x(index) + left.size();
                move(y, x);
                break;

            case KEY_BACKSPACE:
                if (left.empty()) {
                    move(y, x);
                } else {
                    left.pop_back();
                    x--;
                    printw("%s ", right.c_str());
                    move(y, x);
                }
                break;

            default:
                x++;
                left += c;
                if (!right.empty()) {
                    printw("%s", right.c_str());
                    move(y, x);
                }
        }

    }

break2:
    endwin();

    return EXIT_SUCCESS;
}
