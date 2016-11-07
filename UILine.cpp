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

        //TODO: implement commands

        add_history(line);
        free(line);
    }

    (void)write_history(path);
    free(path);

    return EXIT_SUCCESS;
}
