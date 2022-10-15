#include "have_vim.h"
#include <stdbool.h>
#include <stdlib.h>

bool have_vim(void) {
  return system("which vim >/dev/null 2>/dev/null") == EXIT_SUCCESS;
}
