#include <clink/clink.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int print(const char *line) {
  if (fputs(line, stdout) == EOF)
    return errno;

  // the last file line may not be terminated, so ensure it displays correctly
  if (line[strlen(line) - 1] != '\n')
    putchar('\n');

  return 0;
}

int main(int argc, char **argv) {

  if (argc != 2) {
    fprintf(stderr, "usage: %s filename\n", argv[0]);
    return EXIT_FAILURE;
  }

  int rc = clink_vim_read(argv[1], print);
  if (rc != 0) {
    fprintf(stderr, "failed: %s\n", strerror(rc));
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
