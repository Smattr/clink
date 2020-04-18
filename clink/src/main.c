#include <assert.h>
#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include "option.h"
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

static void parse_args(int argc, char **argv) {

  for (;;) {
    static const struct option opts[] = {
      {"build-only",    no_argument,       0, 'b'},
      {"color",         required_argument, 0, 128 },
      {"colour",        required_argument, 0, 128 },
      {"database",      required_argument, 0, 'f'},
      {"include",       required_argument, 0, 'I'},
      {"jobs",          required_argument, 0, 'j'},
      {"line-oriented", no_argument,       0, 'l'},
      {"no-build",      no_argument,       0, 'd'},
      {0, 0, 0, 0},
    };

    int index = 0;
    int c = getopt_long(argc, argv, "bCcdef:I:j:lqRs:T", opts, &index);

    if (c == -1)
      break;

    switch (c) {

      case 'b': // --build-only
        option.ncurses_ui = false;
        option.line_ui = false;
        break;

      case 'C':
        fprintf(stderr, "Clink has no equivalent of Cscope's -C option\n");
        exit(EXIT_FAILURE);

      case 'c':
        // -c (use only ASCII in cross-ref) has no relevance for Clink
        break;

      case 'd': // --no-build
        option.update_database = false;
        break;

      case 'e':
        fprintf(stderr, "Clink has no equivalent of Cscope's -e option\n");
        exit(EXIT_FAILURE);

      case 'f': // --database
        free(option.database_path);
        // if this is a relative path, make it absolute
        if (optarg[0] != '/') {
          char *cwd = getcwd(NULL, 0);
          if (cwd == NULL ||
              asprintf(&option.database_path, "%s/%s", cwd, optarg) < 0) {
            fprintf(stderr, "out of memory\n");
            exit(EXIT_FAILURE);
          }
          free(cwd);
        } else {
          option.database_path = xstrdup(optarg);
        }
        break;

      case 'I': // --include
        xappend(&option.cxx_argv, &option.cxx_argc, "-I");
        xappend(&option.cxx_argv, &option.cxx_argc, optarg);
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

      case 'q':
        // -q (build inverted index) has no relevance for Clink
        break;

      case 'R':
        // -R is irrelevant because Clink always recurses into subdirectories
        break;

      case 's':
        xappend(&option.src, &option.src_len, optarg);
        break;

      case 'T':
        fprintf(stderr, "Clink has no equivalent of Cscope's -T option\n");
        exit(EXIT_FAILURE);

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

done:
  clean_up_options();

  return rc ? EXIT_FAILURE : EXIT_SUCCESS;
}
