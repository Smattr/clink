#include "run.h"
#include <clink/vim.h>
#include <errno.h>
#include <stddef.h>

int clink_editor_open(const char *editor, const char *filename) {

  if (editor == NULL)
    return EINVAL;

  if (filename == NULL)
    return EINVAL;

  const char *argv[] = {editor, "--", filename, NULL};

  return run(argv);
}
