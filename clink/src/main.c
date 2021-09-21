#include <assert.h>
#include "build.h"
#include "../../common/compiler.h"
#include <clink/clink.h>
#include <errno.h>
#include <getopt.h>
#include "help.h"
#include <limits.h>
#include "line_ui.h"
#include "ncurses_ui.h"
#include "option.h"
#include "path.h"
#include "sigint.h"
#include <sqlite3.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static char *xstrdup(const char *s) {
  char *p = strdup(s);
  if (p == NULL) {
    fprintf(stderr, "out of memory\n");
    exit(EXIT_FAILURE);
  }
  return p;
}

static void xappend(char ***list, size_t *len, const char *item) {

  // expand the list
  *list = realloc(*list, (*len + 1) * sizeof(**list));
  if (*list == NULL) {
    fprintf(stderr, "out of memory\n");
    exit(EXIT_FAILURE);
  }
  ++(*len);

  // append the new item
  (*list)[*len - 1] = xstrdup(item);
}

/// list of user includes (`--include …` or `-I…`)
static char **includes;
static size_t includes_len;

static void parse_args(int argc, char **argv) {

  while (true) {
    static const struct option opts[] = {
      {"build-only",    no_argument,       0, 'b'},
      {"color",         required_argument, 0, 128 },
      {"colour",        required_argument, 0, 128 },
      {"database",      required_argument, 0, 'f'},
      {"debug",         no_argument,       0, 129 },
      {"help",          no_argument,       0, 'h'},
      {"include",       required_argument, 0, 'I'},
      {"jobs",          required_argument, 0, 'j'},
      {"line-oriented", no_argument,       0, 'l'},
      {"no-build",      no_argument,       0, 'd'},
      {0, 0, 0, 0},
    };

    int index = 0;
    int c = getopt_long(argc, argv, "bdf:hI:j:l", opts, &index);

    if (c == -1)
      break;

    switch (c) {

      case 'b': // --build-only
        option.ncurses_ui = false;
        option.line_ui = false;
        break;

      case 'd': // --no-build
        option.update_database = false;
        break;

      case 'f': // --database
        free(option.database_path);
        option.database_path = xstrdup(optarg);
        break;

      case 'h': // --help
        help();
        exit(EXIT_SUCCESS);

      case 'I': // --include
        xappend(&includes, &includes_len, optarg);
        break;

      case 'j': // --jobs
        if (strcmp(optarg, "auto") == 0) {
          option.threads = 0;
        } else {
          char *endptr;
          option.threads = strtoul(optarg, &endptr, 0);
          if (optarg == endptr || (option.threads == ULONG_MAX &&
                  errno == ERANGE)) {
            fprintf(stderr, "illegal value to --jobs: %s\n", optarg);
            exit(EXIT_FAILURE);
          }
        }
        break;

      case 'l':
        option.ncurses_ui = false;
        option.line_ui = true;
        break;

      case 128: // --colour
        if (strcmp(optarg, "auto") == 0) {
          option.colour = AUTO;
        } else if (strcmp(optarg, "always") == 0) {
          option.colour = ALWAYS;
        } else if (strcmp(optarg, "never") == 0) {
          option.colour = NEVER;
        } else {
          fprintf(stderr, "illegal value to --colour: %s\n", optarg);;
          exit(EXIT_FAILURE);
        }
        break;

      case 129: // --debug
        option.debug = true;
        break;

      default:
        exit(EXIT_FAILURE);
    }
  }

  // interpret any remaining options as sources
  for (size_t i = optind; i < (size_t)argc; ++i)
    xappend(&option.src, &option.src_len, argv[i]);

  // if the user wanted automatic parallelism, give them a thread per core
  if (option.threads == 0) {
    long r = sysconf(_SC_NPROCESSORS_ONLN);
    if (r < 1) {
      option.threads = 1;
    } else {
      option.threads = (unsigned long)r;
    }
  }

  // if the user wanted auto colour, make a decision based on whether stdout is
  // a TTY
  if (option.colour == AUTO)
    option.colour = isatty(STDOUT_FILENO) ? ALWAYS : NEVER;
  assert(option.colour == ALWAYS || option.colour == NEVER);

  // at most one user interface should have been enabled
  assert(!option.ncurses_ui || !option.line_ui);
}

int main(int argc, char **argv) {

  // parse command line arguments
  parse_args(argc, argv);

  // figure out where to create (or re-open) .clink.db
  int rc = set_db_path();
  if (rc) {
    fprintf(stderr, "failed to configure path to database: %s\n", strerror(rc));
    goto done;
  }
  assert(option.database_path != NULL);

  // figure out what source paths we should scan
  if ((rc = set_src())) {
    fprintf(stderr, "failed to set source files/directories to scan: %s\n",
      strerror(rc));
    goto done;
  }
  assert(option.src != NULL && option.src_len > 0);

  // setup `-I`/`-isystem` options
  rc = set_cxx_flags(includes, includes_len);
  // we do not need the includes collection any more
  for (size_t i = 0; i < includes_len; ++i)
    free(includes[i]);
  free(includes);
  includes = NULL;
  includes_len = 0;
  if (UNLIKELY(rc)) {
    fprintf(stderr, "failed to set CXX flags: %s\n", strerror(rc));
    goto done;
  }

  if (option.update_database) {
    for (size_t i = 0; i < option.src_len; ++i) {

      // check the source path exists, to avoid later complications
      if (access(option.src[i], R_OK) < 0) {
        rc = errno;
        fprintf(stderr, "%s not accessible: %s\n", option.src[i], strerror(rc));
        goto done;
      }

      // make the path absolute to ease later work
      char *absolute = realpath(option.src[i], NULL);
      if (absolute == NULL) {
        rc = errno;
        fprintf(stderr, "failed to make %s absolute: %s\n", option.src[i],
          strerror(rc));
        goto done;
      }
      free(option.src[i]);
      option.src[i] = absolute;
    }
  }

  // ensure SQLite is safe to use multi-threaded
  if (option.threads > 1) {
    if (!sqlite3_threadsafe()) {
      fprintf(stderr, "your SQLite library does not support multi-threading\n");
      rc = -1;
      goto done;
    }
    int r = sqlite3_config(SQLITE_CONFIG_SERIALIZED);
    if (r != SQLITE_OK) {
      fprintf(stderr, "failed to set SQLite serialized mode: %s\n",
              sqlite3_errstr(r));
      rc = -1;
      goto done;
    }
  }

  // block SIGINT while we open (and possibly construct) the database, so we do
  // not end up corrupting the file if we are interrupted
  if ((rc = sigint_block())) {
    fprintf(stderr, "failed to block SIGINT: %s\n", strerror(rc));
    goto done;
  }

  // open the database
  clink_db_t *db = NULL;
  if ((rc = clink_db_open(&db, option.database_path))) {
    fprintf(stderr, "failed to open database: %s\n", strerror(rc));
    goto done;
  }

  // now that the database exists, we can use realpath() on its path to make
  // sure it is absolute to simplify some later operation
  {
    char *a = realpath(option.database_path, NULL);
    if (a == NULL) {
      rc = errno;
      fprintf(stderr, "failed to make %s absolute: %s\n", option.database_path,
        strerror(rc));
      goto done1;
    }
    free(option.database_path);
    option.database_path = a;
  }

  // we can now be safely interrupted
  (void)sigint_unblock();

  // build/update the database, if requested
  if (option.update_database) {
    if ((rc = build(db)))
      goto done1;
  }

  // run line-oriented interface, if requested
  if (option.line_ui) {
    if ((rc = line_ui(db)))
      goto done1;
  }

  // Ncurses interface, if requested
  if (option.ncurses_ui) {
    if ((rc = ncurses_ui(db)))
      goto done1;
  }

done1:
  clink_db_close(&db);
done:
  clean_up_options();

  return rc ? EXIT_FAILURE : EXIT_SUCCESS;
}
