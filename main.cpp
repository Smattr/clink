#include "AsmParser.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include "CXXParser.h"
#include "Database.h"
#include <dirent.h>
#include <errno.h>
#include "WorkQueue.h"
#include <functional>
#include <getopt.h>
#include <iostream>
#include <limits.h>
#include "Options.h"
#include "Resources.h"
#include "Symbol.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <thread>
#include "UICurses.h"
#include "UILine.h"
#include <unistd.h>
#include "util.h"

using namespace std;

static const char default_database[] = ".clink.db";

static void usage(const char *progname) {
    cerr << "usage: " << progname << " [options]\n"
         << "\n"
         << " --file FILE | -f FILE    database to use (.clink.db by default)\n"
         << " --jobs JOBS | -j JOBS    set number of threads to use (0 or "
            "\"auto\" for the default, which is the number of cores)\n";
}

/* Default options. */
Options opts = {
    .database = default_database,
    .update_database = true,
    .ui = UI_CURSES,
    .threads = 0,
};

static void parse_options(int argc, char **argv) {
    for (;;) {
        static const struct option options[] = {
            {"file", required_argument, 0, 'f'},
            {"jobs", required_argument, 0, 'j'},
            {"line-oriented", no_argument, 0, 'l'},
            {0, 0, 0, 0},
        };

        int index = 0;
        int c = getopt_long(argc, argv, "bdf:j:l", options, &index);

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

            case 'j':
                if (strcmp(optarg, "auto") == 0) {
                    opts.threads = 0;
                } else {
                    char *endptr;
                    opts.threads = strtoul(optarg, &endptr, 0);
                    if (optarg == endptr || (opts.threads == ULONG_MAX &&
                            errno == ERANGE)) {
                        cerr << "illegal value to --jobs\n";
                        exit(EXIT_FAILURE);
                    }
                }
                break;

            case 'l':
                opts.ui = UI_LINE;
                break;

            default:
                usage(argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    // If the user wanted automatic parallelism, give them a thread per core.
    if (opts.threads == 0) {
        unsigned cores = thread::hardware_concurrency();
        if (cores == 0) {
            cerr << "your system appears to have an invalid number of "
                "processors\n";
            exit(EXIT_FAILURE);
        }
        opts.threads = (unsigned long)cores;
    }
}

static void update(SymbolConsumer &db, WorkQueue &fq) {
    Resources r;

    for (;;) {
        string path;
        try {
            path = fq.pop();
        } catch (NoMoreEntries &) {
            break;
        }

        db.purge(path);

        // Is this assembly?
        if (ends_with(path.c_str(), ".s") || ends_with(path.c_str(), ".S")) {
            AsmParser *asm_parser = r.get_asm_parser();
            if (!asm_parser->load(path.c_str()))
                continue;
            asm_parser->process(db);
            asm_parser->unload();
        }

        else {
            // OK, it must be C/C++.
            CXXParser *cxx_parser = r.get_cxx_parser();
            if (!cxx_parser->load(path.c_str()))
                continue;
            cxx_parser->process(db);
            cxx_parser->unload();
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

    if (opts.update_database) {

        if (opts.threads == 1) {

            /* When running single-threaded, we can create a thread-unsafe file
             * queue and just directly pump results into the database.
             */

            WorkQueue queue(".", era_start);

            /* Open a transaction before starting to manipulate the database.
             * Repeated insertions without a containing transaction are wrapped in
             * an automatic transaction. Commiting these automatic transactions
             * intolerably slows the database update.
             */
            db.open_transaction();

            /* Scan the directory for files that have changed since the database
             * was last updated.
             */
            update(db, queue);

            db.close_transaction();

        } else {
            assert(opts.threads > 1);

            // Create a single, shared file queue.
            ThreadSafeWorkQueue queue(".", era_start);

            // Create and start N - 1 threads.
            vector<thread*> threads;
            vector<PendingActions*> pending;
            for (unsigned long i = 0; i < opts.threads - 1; i++) {
                PendingActions *pa = new PendingActions();
                thread *t = new thread(update, ref(*pa), ref(queue));
                threads.push_back(t);
                pending.push_back(pa);
            }

            /* Join in ourselves, but let's operate directly on the database to
             * save one set of replay actions.
             */
            db.open_transaction();
            update(db, queue);

            // Clean up all the other threads.
            for (thread *t : threads) {
                t->join();
                delete t;
            }

            // Merge their results.
            for (PendingActions *pa : pending) {
                db.replay(*pa);
                delete pa;
            }

            db.close_transaction();
        }
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
