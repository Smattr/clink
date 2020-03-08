#include <clink/clink.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

  if (argc != 2) {
    fprintf(stderr,
      "usage: %s filename\n"
      " test utility for Clink's assembly parser\n", argv[0]);
    return EXIT_FAILURE;
  }

  int rc = clink_parse_asm(argv[1], printer, NULL);
  if (rc != 0) {
    fprintf(stderr, "%s\n", strerror(rc));
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
