#include <clink/clink.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// file to open
static char *filename;

// command line arguments to pass to Clang
static char **clang_argv;
static size_t clang_argc;

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
        clang_argc += 2;
        clang_argv = realloc(clang_argv, clang_argc * sizeof(clang_argv[0]));
        if (clang_argv == NULL) {
          fprintf(stderr, "out of memory\n");
          exit(EXIT_FAILURE);
        }
        clang_argv[clang_argc - 2] = xstrdup("-I");
        clang_argv[clang_argc - 1] = xstrdup(optarg);
        break;

      case '?':
        fprintf(stderr,
          "usage: %s [-I PATH | --include PATH] filename\n"
          " test utiltiy for Clink's C/C++ parser\n", argv[0]);
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

static int printer(const struct clink_symbol *symbol,
    void *state __attribute__((unused))) {

  printf("%s:%lu:%lu: ", symbol->path, symbol->lineno, symbol->colno);

  switch (symbol->category) {

    case CLINK_DEFINITION:
      printf("definition");
      break;

    case CLINK_FUNCTION_CALL:
      printf("function call");
      break;

    case CLINK_REFERENCE:
      printf("reference");
      break;

    case CLINK_INCLUDE:
      printf("include");
      break;
  }

  printf(" of %.*s\n", (int)symbol->name_len, symbol->name);

  return 0;
}

int main(int argc, char **argv) {

  // parse command line arguments
  parse_args(argc, argv);

  int rc = clink_parse_c(filename, (const char**)clang_argv, clang_argc,
    printer, NULL);

  // clean up
  free(filename);
  for (size_t i = 0; i < clang_argc; i++)
    free(clang_argv[i]);
  free(clang_argv);

  if (rc != 0) {
    fprintf(stderr, "%s\n", strerror(rc));
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
