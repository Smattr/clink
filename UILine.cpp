#include <cstdlib>
#include <ctype.h>
#include "Database.h"
#include <iostream>
#include <readline/history.h>
#include <readline/readline.h>
#include "UILine.h"
#include <unistd.h>

using namespace std;

int UILine::run(Database &db) {

    // Set up history for readline.
    const char *home = getenv("HOME");
    char *path;
    if (asprintf(&path, "%s/.clink_history", home) < 0) {
        cerr << "out of memory\n";
        return EXIT_FAILURE;
    }
    (void)read_history(path); // Ignore failure

    int ret = EXIT_SUCCESS;

    char *line;
    while ((line = readline("clink> "))) {

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

            case '0': // find symbol
                break;

            case '1': // find definition
                break;

            case '2': // find callees
                break;

            case '3': // find callers
                break;

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

        add_history(line);
        free(line);
    }

break2:
    (void)write_history(path);
    free(path);

    return ret;
}
