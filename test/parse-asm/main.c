#include <clink/clink.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {

  if (argc != 2) {
    fprintf(stderr, "usage: %s filename\n"
                 " test utility for Clink's assembly parser\n", argv[0]);
    return EXIT_FAILURE;
  }

  // setup an iterator for parsing the source file
  clink_iter_t *it = NULL;
  int rc = clink_parse_asm(&it, argv[1]);
  if (rc) {
    fprintf(stderr, "clink_parse_asm: %s\n", strerror(rc));
    return EXIT_FAILURE;
  }

  while (clink_iter_has_next(it)) {

    const clink_symbol_t *s = NULL;
    if ((rc = clink_iter_next_symbol(it, &s))) {
      fprintf(stderr, "clink_iter_next_symbol: %s\n", strerror(rc));
      goto done;
    }

    printf("%s:%lu:%lu: ", s->path, s->lineno, s->colno);

    switch (s->category) {

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

    printf(" of %s\n", s->name);
  }

done:
  clink_iter_free(&it);

  return rc == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
