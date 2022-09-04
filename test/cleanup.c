#include "test.h"
#include <stdlib.h>

cleanup_t *cleanups;

void run_cleanups(void) {
  while (cleanups != NULL) {
    cleanups->function(cleanups->arg);
    cleanup_t *c = cleanups->next;
    free(cleanups);
    cleanups = c;
  }
}
