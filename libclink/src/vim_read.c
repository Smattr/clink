#include "debug.h"
#include <clink/vim.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <vimcat/vimcat.h>

int clink_vim_read(const char *filename,
                   int (*callback)(void *state, const char *line),
                   void *state) {

  if (ERROR(filename == NULL))
    return EINVAL;

  if (ERROR(callback == NULL))
    return EINVAL;

  return vimcat_read(filename, callback, state);
}
