#include <clink/clink.h>
#include <errno.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int print(void *ignored, const char *line) {

  (void)ignored;

  if (fputs(line, stdout) == EOF)
    return errno;

  // the last file line may not be terminated, so ensure it displays correctly
  if (line[strlen(line) - 1] != '\n')
    putchar('\n');

  return 0;
}

int main(int argc, char **argv) {

  bool debug = false;

  while (true) {
    static const struct option opts[] = {
        // clang-format off
        {"debug", no_argument, 0, 'd'},
        {"help",  no_argument, 0, 'h'},
        {0, 0, 0, 0},
        // clang-format on
    };

    int index = 0;
    int c = getopt_long(argc, argv, "dh", opts, &index);

    if (c == -1)
      break;

    switch (c) {

    case 'd': // --debug
      debug = true;
      break;

    case 'h': // --help
      fprintf(stderr, "usage: %s [--debug] [filename1 [filename2 [...]]]\n",
              argv[0]);
      return EXIT_SUCCESS;

    default:
      return EXIT_FAILURE;
    }
  }

  if (debug)
    clink_debug_on();

  for (size_t i = optind; i < (size_t)argc; ++i) {
    int rc = clink_vim_read(argv[i], print, NULL);
    if (rc != 0) {
      fprintf(stderr, "failed: %s\n", strerror(rc));
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
