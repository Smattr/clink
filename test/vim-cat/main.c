#include <clink/clink.h>
#include <stdio.h>
#include <stdlib.h>

static int print(const char *line) {
  printf("%s", line);
  return 0;
}

int main(int argc, char **argv) {

  if (argc != 2) {
    fprintf(stderr, "usage: %s filename\n", argv[0]);
    return EXIT_FAILURE;
  }

  const char *filename = argv[1];

  return clink_vim_highlight(filename, &print);
}
