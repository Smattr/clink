#include <cassert>
#include <cstdlib>
#include <curses.h>
#include "Database.h"
#include <string>
#include <string.h>
#include "UICurses.h"

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
