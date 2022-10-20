/// \file
/// \brief testing utility for `find_me`

#include "../../clink/src/find_me.h"
#include <stdio.h>
#include <stdlib.h>

int main(void) {

  char *me = find_me();

  if (me == NULL) {
    fprintf(stderr, "could not locate myself\n");
    return EXIT_FAILURE;
  }

  printf("I am %s\n", me);
  free(me);
  return EXIT_SUCCESS;
}
