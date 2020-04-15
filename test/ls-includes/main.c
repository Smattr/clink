#include <clink/clink.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {

  if (argc > 2 || (argc == 2 && strcmp(argv[1], "--help") == 0)) {
    fprintf(stderr, "usage: %s [compiler (default $CXX)]\n"
                    " list #include paths used by a C++ compiler\n", argv[0]);
    return EXIT_FAILURE;
  }

  const char *compiler = argc == 2 ? argv[1] : NULL;

  char **includes = NULL;
  size_t includes_len = 0;
  int rc = clink_compiler_includes(compiler, &includes, &includes_len);

  if (rc) {
    fprintf(stderr, "clink_compiler_includes: %s\n", strerror(rc));
    return EXIT_FAILURE;
  }

  for (size_t i = 0; i < includes_len; ++i)
    printf("%s\n", includes[i]);

  for (size_t i = 0; i < includes_len; ++i)
    free(includes[i]);
  free(includes);

  return EXIT_SUCCESS;
}
