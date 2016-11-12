#include <cstdlib>
#include <ctype.h>
#include "Database.h"
#include <iostream>
#include <readline/readline.h>
#include "Symbol.h"
#include "UILine.h"
#include <unistd.h>
#include <vector>

using namespace std;

static const char *lstrip(const char *s) {

    if (!s)
        return "\n";

    const char *t = s;
    while (isspace(*t))
        t++;

    if (*t == '\0')
        return "\n";

    return t;
}

static void print_leader(const vector<Symbol> &vs) {
    cout << "cscope: " << vs.size() << " lines\n";
}

int UILine::run(Database &db) {

    int ret = EXIT_SUCCESS;

    char *line;
    while ((line = readline(">> "))) {

        // Skip leading white space
        char *command = line;
        while (isspace(*command))
            command++;

        // Ignore blank lines
        if (strcmp(command, "") == 0) {
            free(line);
            continue;
        }

        switch (*command) {

            case '0': { // find symbol
                vector<Symbol> vs = db.find_symbol(command + 1);
                print_leader(vs);
                for (auto &&s : vs) {
                    cout << s.path() << " " << s.parent() << " " << s.line() <<
                        " " << lstrip(s.context());
                }
                break;
            }

            case '1': { // find definition
                vector<Symbol> vs = db.find_definition(command + 1);
                print_leader(vs);
                for (auto &&s : vs) {
                    cout << s.path() << " " << (command + 1) << " " <<
                        s.line() << " " << lstrip(s.context());
                }
                break;
            }

            case '2': { // find calls
                vector<Symbol> vs = db.find_call(command + 1);
                print_leader(vs);
                for (auto &&s : vs) {
                    cout << s.path() << " " << s.name() << " " << s.line() <<
                        " " << lstrip(s.context());
                }
                break;
            }

            case '3': { // find callers
                vector<Symbol> vs = db.find_caller(command + 1);
                print_leader(vs);
                for (auto &&s : vs) {
                    cout << s.path() << " " << s.parent() << " " << s.line() <<
                        " " << lstrip(s.context());
                }
                break;
            }

            case '7': // find file
                break;

            case '8': // find includers
                break;

            // Commands we don't support. Just pretend there were no results.
            case '4': // find text
            case '5': // change text
            case '6': // find pattern
            case '9': // find assignments
                cout << "cscope: 0 lines\n";
                break;

            /* Bail out on any unrecognised command, under the assumption Vim
             * would never send us a malformed command.
             */
            default:
                free(line);
                ret = EXIT_FAILURE;
                goto break2;

        }

        free(line);
    }

break2:

    return ret;
}
