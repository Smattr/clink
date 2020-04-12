#include <clink/clink.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {

  if (argc != 2) {
    fprintf(stderr, "usage: %s filename\n", argv[0]);
    return EXIT_FAILURE;
  }

  int rc = 0;

  clink_iter_t *it = NULL;
  if ((rc = clink_vim_highlight(&it, argv[1]))) {
    fprintf(stderr, "failed to create iterator: %s\n", strerror(rc));
    return EXIT_FAILURE;
  }

  while (clink_iter_has_next(it)) {

    const char *line = NULL;
    if ((rc = clink_iter_next_str(it, &line))) {
      fprintf(stderr, "failed to retrieve line: %s\n", strerror(rc));
      goto done;
    }

    printf("%s", line);
  }

done:
  clink_iter_free(&it);

  return rc ? EXIT_FAILURE : EXIT_SUCCESS;
}
