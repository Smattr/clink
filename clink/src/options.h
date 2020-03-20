#pragma once

#include <stdbool.h>
#include <stddef.h>

/** a user interface selection
 */
enum interface {
  CURSES = 0, ///< text user interface using Curses
  LOI = 1,    ///< line-oriented interface
  NONE = 2,   ///< no interface (exit after database update)
};

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

  /// which user interface to open
  enum interface interface;

  // skip updating the database?
  bool no_database_update;
};

extern struct options options;
