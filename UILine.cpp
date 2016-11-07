#include <cstdlib>
#include <ctype.h>
#include "Database.h"
#include <iostream>
#include <readline/history.h>
#include <readline/readline.h>
#include "Symbol.h"
#include "UILine.h"
#include <unistd.h>
#include <vector>

using namespace std;

static char *get_file_line(const char *path, unsigned lineno) {

    FILE *f = fopen(path, "r");
    if (f == nullptr)
        return nullptr;

    char *line = nullptr;
    size_t size;
    while (lineno > 0) {
        if (getline(&line, &size, f) < 0) {
            free(line);
            line = nullptr;
            break;
        }
        lineno--;
    }

    fclose(f);

    return line;
}

int UILine::run(Database &db) {

    /* Though we're not expecting a human to use this interface, give them a
     * useful history just in case.
     */
    const char *home = getenv("HOME");
    char *path;
    if (asprintf(&path, "%s/.clink_history", home) < 0) {
        cerr << "out of memory\n";
        return EXIT_FAILURE;
    }
    (void)read_history(path); // Ignore failure

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
                vector<Symbol> vs = db.find_symbols(command + 1);
                cout << "cscope " << vs.size() << " lines\n";
                for (auto sym : vs) {
                    char *text = get_file_line(sym.path, sym.line);
                    cout << sym.path << " " << (sym.parent ? sym.parent : "<global>") << " " << sym.line << " ";
                    if (text) {
                        char *p = text;
                        while (isspace(*p))
                            p++;
                        cout << (*p == '\0' ? "\n" : p);
                        free(text);
                    } else {
                        cout << "\n";
                    }
                    free((void*)sym.path);
                    free((void*)sym.parent);
                }
                break;
            }

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
