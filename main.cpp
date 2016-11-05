#include "CXXParser.h"
#include "Database.h"
#include <getopt.h>
#include <iostream>
#include "Symbol.h"
#include <unistd.h>

using namespace std;

static const char default_database[] = ".clink.db";

static void usage(const char *progname) {
    cerr << "usage: " << progname << " [options]\n"
         << "\n"
         << " --file FILE | -f FILE    database to use (.clink.db by default)\n";
}

/* Default options. */
static struct {
    const char *database;
} opts = {
    .database = default_database,
};

static void parse_options(int argc, char **argv) {
    for (;;) {
        static const struct option options[] = {
            {"file", required_argument, 0, 'f'},
            {0, 0, 0, 0},
        };

        int index = 0;
        int c = getopt_long(argc, argv, "f:", options, &index);

        if (c == -1)
            break;

        switch (c) {
            case 'f':
                opts.database = optarg;
                break;

            default:
                usage(argv[0]);
                exit(EXIT_FAILURE);
        }
    }
}

int main(int argc, char **argv) {

    parse_options(argc, argv);

    Database db;
    if (!db.open(opts.database)) {
        cerr << "failed to open " << opts.database << "\n";
        return EXIT_FAILURE;
    }

    CXXParser parser;
    if (!parser.load("foo.cpp")) {
        cerr << "failed to load foo.cpp\n";
        return EXIT_FAILURE;
    }

    parser.process(db);

    return EXIT_SUCCESS;
}
