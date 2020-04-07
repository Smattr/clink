#pragma once

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

enum Colour {
  AUTO,
  ALWAYS,
  NEVER,
};

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

  // colour terminal output on or off
  Colour colour = AUTO;

  // directories to look in for #include files
  std::vector<std::string> include_dirs;
};

extern Options options;
