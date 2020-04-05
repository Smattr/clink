#include "Options.h"
#include <string>
#include <vector>

Options options = {
  .database_path = {},
  .update_database = true,
  .ui = UI_CURSES,
  .threads = 0,
  .include_dirs = std::vector<std::string>(),
};
