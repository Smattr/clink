#include <errno.h>
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
      { "color",     no_argument,       0, 129 },
      { "colour",    no_argument,       0, 129 },
      { "help",      no_argument,       0, 'h' },
      { "jobs",      required_argument, 0, 'j' },
      { "no-color",  no_argument,       0, 130 },
      { "no-colour", no_argument,       0, 130 },
      { "threads",   required_argument, 0, 'j' },
      { 0, 0, 0, 0 },
    };

    int index = 0;
    int c = getopt_long(argc, argv, "hj:", opts, &index);

    if (c == -1)
      break;

    switch (c) {

      case 129: // --colour
        options.no_colour = false;
        break;

      case 'h': // --help
        printf("TODO\n");
        exit(EXIT_SUCCESS);

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

      case 130: // --no-colour
        options.no_colour = true;
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

  parse_args(argc, argv);

  return EXIT_SUCCESS;
}
