#pragma once

#if 0
#include <string>
#include <vector>

typedef enum {
  UI_NONE,
  UI_CURSES,
  UI_LINE,
} ui_t;

struct Options {
  const char *database;
  bool update_database;
  ui_t ui;

  // Parallelism (0 == auto).
  unsigned long threads;

  // Directories to look in for #include files
  std::vector<std::string> include_dirs;
};

extern Options opts;
#endif

#include <stdbool.h>
#include <stddef.h>

struct options {

  // path to the database
  char *database_filename;

  // how many threads to use (0 == auto)
  unsigned long jobs;

  // directories to search for #included files
  char **include_paths;
  size_t include_paths_size;

  // do not use ANSI terminal colours?
  bool no_colour;

  // use line-oriented interface instead of Curses?
  bool line_oriented;
};

extern struct options options;
