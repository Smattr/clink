#include <assert.h>
#include "build.h"
#include <clink/clink.h>
#include <errno.h>
#include "get_db_path.h"
#include <getopt.h>
#include <limits.h>
#include "options.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void parse_args(int argc, char **argv) {

  for (;;) {
    static const struct option opts[] = {
      { "build-database-only", no_argument,       0, 'b' },
      { "color",               no_argument,       0, 129 },
      { "colour",              no_argument,       0, 129 },
      { "help",                no_argument,       0, 'h' },
      { "jobs",                required_argument, 0, 'j' },
      { "line-oriented",       no_argument,       0, 'l' },
      { "no-color",            no_argument,       0, 130 },
      { "no-colour",           no_argument,       0, 130 },
      { "no-update-database",  no_argument,       0, 'd' },
      { "threads",             required_argument, 0, 'j' },
      { "update-database",     no_argument,       0, 131 },
      { 0, 0, 0, 0 },
    };

    int index = 0;
    int c = getopt_long(argc, argv, "hj:", opts, &index);

    if (c == -1)
      break;

    switch (c) {

      case 'b': // --build-database-only
        options.interface = NONE;
        options.no_database_update = false;
        break;

      case 129: // --colour
        options.no_colour = false;
        break;

      case 'h': // --help
        printf("TODO\n");
        exit(EXIT_SUCCESS);

      case 'l': // --line-oriented
        options.interface = LOI;
        break;

      case 130: // --no-colour
        options.no_colour = true;
        break;

      case 'd': // --no-update-database
        options.no_database_update = true;
        break;

      case 'j': // --jobs
        if (strcmp(optarg, "auto") == 0) {
          options.jobs = 0;
        } else {
          char *endptr;
          options.jobs = strtoul(optarg, &endptr, 0);
          if (optarg == endptr ||
              (options.jobs == ULONG_MAX && errno == ERANGE)) {
            fprintf(stderr, "illegal value \"%s\" to --jobs\n", optarg);
            exit(EXIT_FAILURE);
          }
        }
        break;

      case 131: // --update-database
        options.no_database_update = false;
        break;

      case '?':
        fprintf(stderr, "run %s --help to see available options\n", argv[0]);
        exit(EXIT_FAILURE);
    }
  }

  // if the user wanted automatic parallelism, give them a thread per core
  if (options.jobs == 0) {
    long r = sysconf(_SC_NPROCESSORS_ONLN);
    options.jobs = r < 1 ? 1 : (unsigned long)r;
  }
}

int main(int argc, char **argv) {

  // parse command line arguments
  parse_args(argc, argv);

  // find and open the symbol database
  struct clink_db db = { 0 };
  {
    char *db_path = NULL;
    int rc = get_db_path(&db_path);
    if (rc != 0) {
      fprintf(stderr, "failed to locate database path: %s\n", strerror(rc));
      return EXIT_FAILURE;
    }

    rc = clink_db_open(&db, db_path);
    free(db_path); // no longer needed
    if (rc != 0) {
      fprintf(stderr, "failed to open database: %s\n", clink_strerror(rc));
      return EXIT_FAILURE;
    }
  }

  // were we asked to build/update the database?
  if (!options.no_database_update) {
    int rc = build(&db);
    if (rc != 0) {
      fprintf(stderr, "error: %s\n", clink_strerror(rc));
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
