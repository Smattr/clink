#include <clink/cscope.h>
#include <stdbool.h>
#include <stdlib.h>

bool clink_have_cscope(void) {
  return system("which cscope >/dev/null 2>/dev/null") == EXIT_SUCCESS;
}
