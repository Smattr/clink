#include <clink/clink.h>
#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// file to open
static char *filename;

// list of command line arguments to pass to Clang
static size_t clang_argc;
static char **clang_argv;

static void *xrealloc(void *p, size_t s) {
  void *ptr = realloc(p, s);
  if (ptr == NULL) {
    fprintf(stderr, "out of memory\n");
    exit(EXIT_FAILURE);
  }
  return ptr;
}

static char *xstrdup(const char *s) {
  char *p = strdup(s);
  if (p == NULL) {
    fprintf(stderr, "out of memory\n");
    exit(EXIT_FAILURE);
  }
  return p;
}

static void parse_args(int argc, char **argv) {

  for (;;) {

    static struct option opts[] = {
      { "include", required_argument, 0, 'I' },
      { 0, 0, 0, 0 },
    };

    int option_index = 0;
    int c = getopt_long(argc, argv, "I:", opts, &option_index);

    if (c == -1)
      break;

    switch (c) {

      case 'I':
        // expand clang_argv
        clang_argc += 2;
        clang_argv = xrealloc(clang_argv, clang_argc * sizeof(clang_argv[0]));

        // add arguments to include the given directory
        clang_argv[clang_argc - 2] = xstrdup("-I");
        clang_argv[clang_argc - 1] = xstrdup(optarg);

        break;

      case '?':
        fprintf(stderr, "usage: %s [-I PATH | --include PATH] filename\n"
                        " test utiltiy for Clink's C/C++ parser\n",
                        argv[0]);
        exit(EXIT_FAILURE);

    }
  }

  if (optind < argc - 1) {
    fprintf(stderr, "parsing multiple files is not supported\n");
    exit(EXIT_FAILURE);
  }

  if (optind == argc - 1)
    filename = xstrdup(argv[optind]);

  if (filename == NULL) {
    fprintf(stderr, "input file is required\n");
    exit(EXIT_FAILURE);
  }
}

int main(int argc, char **argv) {

  // parse command line arguments
  parse_args(argc, argv);

  // create an iterator for parsing the input file
  int rc = 0;
  clink_iter_t *it = NULL;
  if ((rc = clink_parse_c(&it, filename, clang_argc, (const char**)clang_argv))) {
    fprintf(stderr, "clink_parse_c: %s\n", strerror(rc));
    goto done;
  }

  // loop through symbols, printing them
  while (clink_iter_has_next(it)) {

    const clink_symbol_t *sym = NULL;
    if ((rc = clink_iter_next_symbol(it, &sym))) {
      fprintf(stderr, "clink_iter_next_symbol: %s\n", strerror(rc));
      goto done;
    }

    printf("%s:%lu:%lu: ", sym->path, sym->lineno, sym->colno);

    switch (sym->category) {

      case CLINK_DEFINITION:
        printf("definition");
        break;

      case CLINK_INCLUDE:
        printf("include");
        break;

      case CLINK_FUNCTION_CALL:
        printf("function call");
        break;

      case CLINK_REFERENCE:
        printf("reference");
        break;

    }

    printf(" of %s\n", sym->name);
  }

done:
  // clean up
  clink_iter_free(&it);
  for (size_t i = 0; i < clang_argc; ++i)
    free(clang_argv[i]);
  free(clang_argv);
  free(filename);

  return rc == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
