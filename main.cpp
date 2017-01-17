#include <cstdio>
#include <cstring>
#include "CXXParser.h"
#include "Database.h"
#include <dirent.h>
#include "FileQueue.h"
#include <getopt.h>
#include <iostream>
#include "Options.h"
#include "Symbol.h"
#include <sys/stat.h>
#include <sys/types.h>
#include "UICurses.h"
#include "UILine.h"
#include <unistd.h>

using namespace std;

static const char default_database[] = ".clink.db";

static void usage(const char *progname) {
    cerr << "usage: " << progname << " [options]\n"
         << "\n"
         << " --file FILE | -f FILE    database to use (.clink.db by default)\n";
}

/* Default options. */
Options opts = {
    .database = default_database,
    .update_database = true,
    .ui = UI_CURSES,
};

static void parse_options(int argc, char **argv) {
    for (;;) {
        static const struct option options[] = {
            {"file", required_argument, 0, 'f'},
            {"line-oriented", no_argument, 0, 'l'},
            {0, 0, 0, 0},
        };

        int index = 0;
        int c = getopt_long(argc, argv, "bdf:l", options, &index);

        if (c == -1)
            break;

        switch (c) {
            case 'b':
                opts.ui = UI_NONE;
                break;

            case 'd':
                opts.update_database = false;
                break;

            case 'f':
                opts.database = optarg;
                break;

            case 'l':
                opts.ui = UI_LINE;
                break;

            default:
                usage(argv[0]);
                exit(EXIT_FAILURE);
        }
    }
}

static void update(Database &db, CXXParser &parser, time_t era_start,
        const string &prefix) {

    FileQueue fq(prefix, era_start);

    for (;;) {
        string path;
        try {
            path = fq.pop();
        } catch (NoMoreEntries &) {
            break;
        }

        db.purge(path);
        if (!parser.load(path.c_str()))
            continue;
        parser.process(db);
        parser.unload();
    }
}

int main(int argc, char **argv) {

    parse_options(argc, argv);

    /* Stat the database to figure out when the last update we did was. */
    time_t era_start;
    struct stat buf;
    if (stat(opts.database, &buf) == 0) {
        era_start = buf.st_mtime;
    } else {
        era_start = 0;
    }

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

    if (opts.update_database) {

        /* Open a transaction before starting to manipulate the database.
         * Repeated insertions without a containing transaction are wrapped in
         * an automatic transaction. Commiting these automatic transactions
         * intolerably slows the database update.
         */
        db.open_transaction();

        /* Scan the directory for files that have changed since the database
         * was last updated.
         */
        update(db, parser, era_start, ".");

        db.close_transaction();
    }

    switch (opts.ui) {
        case UI_LINE: {
            UILine ui;
            return ui.run(db);
        }

        case UI_CURSES: {
            UICurses ui;
            return ui.run(db);
        }

        case UI_NONE:
            // do nothing
            break;
    }

    return EXIT_SUCCESS;
}
