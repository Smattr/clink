#pragma once

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
