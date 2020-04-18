#include "option.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

option_t option = {
  .database_path = NULL,
  .src = NULL,
  .src_len = 0,
  .update_database = true,
  .ncurses_ui = true,
  .line_ui = false,
  .threads = 0,
  .colour = AUTO,
  .cxx_argv = NULL,
  .cxx_argc = 0,
};

void clean_up_options(void) {

  free(option.database_path);
  option.database_path = NULL;

  for (size_t i = 0; i < option.src_len; ++i)
    free(option.src[i]);
  free(option.src);
  option.src = NULL;
  option.src_len = 0;

  for (size_t i = 0; i < option.cxx_argc; ++i)
    free(option.cxx_argv[i]);
  free(option.cxx_argv);
  option.cxx_argv = NULL;
  option.cxx_argc = 0;
}
