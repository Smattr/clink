#include <clink/clink.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {

  if (argc != 2) {
    fprintf(stderr, "usage: %s filename\n", argv[0]);
    return EXIT_FAILURE;
  }

  const char *filename = argv[1];

  char **lines = NULL;
  size_t lines_size = 0;
  int rc = clink_vim_highlight(filename, &lines, &lines_size);
  if (rc != 0)
    return rc;

  for (size_t i = 0; i < lines_size; i++)
    printf("%s", lines[i]);

  // clean up
  for (size_t i = 0; i < lines_size; i++)
    free(lines[i]);
  free(lines);

  return EXIT_SUCCESS;
}
