#include <clink/clink.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {

  if (argc != 4) {
    fprintf(stderr, "usage: %s filename lineno colno\n", argv[0]);
    return EXIT_FAILURE;
  }

  const char *filename = argv[1];

  unsigned long lineno = strtoul(argv[2], NULL, 10);
  if (lineno == 0 || lineno == ULONG_MAX) {
    fprintf(stderr, "invalid line number %s\n", argv[2]);
    return EXIT_FAILURE;
  }

  unsigned long colno = strtoul(argv[3], NULL, 10);
  if (colno == 0 || colno == ULONG_MAX) {
    fprintf(stderr, "invalid column number %s\n", argv[3]);
    return EXIT_FAILURE;
  }

  return clink_vim_open(filename, lineno, colno);
}
