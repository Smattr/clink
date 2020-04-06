#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

struct Options {

  // path to database if it was set on the command line
  std::optional<std::filesystem::path> database_path;

  // update the Clink symbol database with latest source file changes?
  bool update_database = true;

  // run the NCurses-based interface?
  bool ncurses_ui = true;

  // run the line-oriented interface?
  bool line_ui = false;

  // parallelism (0 == auto)
  unsigned long threads;

  // directories to look in for #include files
  std::vector<std::string> include_dirs;
};

extern Options options;
