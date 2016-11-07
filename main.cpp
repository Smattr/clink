#include <cstdio>
#include <cstring>
#include "CXXParser.h"
#include "Database.h"
#include <dirent.h>
#include <getopt.h>
#include <iostream>
#include "Symbol.h"
#include <sys/stat.h>
#include <sys/types.h>
#include "UILine.h"
#include <unistd.h>

using namespace std;

static const char default_database[] = ".clink.db";

static void usage(const char *progname) {
    cerr << "usage: " << progname << " [options]\n"
         << "\n"
         << " --file FILE | -f FILE    database to use (.clink.db by default)\n";
}

typedef enum {
    UI_NONE,
    UI_CURSES,
    UI_LINE,
} ui_t;

/* Default options. */
static struct {
    const char *database;
    bool update_database;
    ui_t ui;
} opts = {
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

static bool ends_with(const char *s, const char *suffix) {
    size_t s_len = strlen(s);
    size_t suffix_len = strlen(suffix);
    return s_len >= suffix_len && strcmp(&s[s_len - suffix_len], suffix) == 0;
}

static void update(Database &db, CXXParser &parser, time_t era_start,
        const char *prefix, DIR *directory) {
    struct dirent entry, *result;
    while (readdir_r(directory, &entry, &result) == 0 && result != nullptr) {

        // If this entry is a directory, descend and scan for contained files.
        if (entry.d_type == DT_DIR && strcmp(entry.d_name, ".") &&
                strcmp(entry.d_name, "..")) {
            char *path;
            if (asprintf(&path, "%s%s/", prefix, entry.d_name) < 0)
                continue;
            DIR *subdir = opendir(path);
            if (subdir != nullptr)
                update(db, parser, era_start, path, subdir);
            closedir(subdir);
            free(path);

        // If this entry is a C/C++ file, scan it for relevant things.
        } else if (entry.d_type == DT_REG && (ends_with(entry.d_name, ".c") ||
                                              ends_with(entry.d_name, ".cpp") ||
                                              ends_with(entry.d_name, ".h") ||
                                              ends_with(entry.d_name, ".hpp"))) {
            char *path;
            if (asprintf(&path, "%s%s", prefix, entry.d_name) < 0)
                continue;
            struct stat buf;
            if (stat(path, &buf) < 0 || buf.st_mtime <= era_start) {
                free(path);
                continue;
            }
            db.purge(path);
            bool ret = parser.load(path);
            free(path);
            if (!ret)
                continue;
            parser.process(db);
            parser.unload();
        }
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
        DIR *dir = opendir(".");
        if (dir == nullptr) {
            cerr << "failed to open directory\n";
            return EXIT_FAILURE;
        }
        update(db, parser, era_start, "", dir);
        closedir(dir);

        db.close_transaction();
    }

    switch (opts.ui) {
        case UI_LINE: {
            UILine ui;
            return ui.run(db);
        }

        case UI_CURSES: {
            // TODO
            return EXIT_SUCCESS;
        }

    }

    return EXIT_SUCCESS;
}
