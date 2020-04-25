#pragma once

#include <stdbool.h>
#include <stddef.h>

typedef enum {
  AUTO,
  ALWAYS,
  NEVER,
} colour_t;

typedef struct {

  // path to database if it was set on the command line
  char *database_path;

  // source files/directories to scan
  char **src;
  size_t src_len;

  // update the Clink symbol database with latest source file changes?
  bool update_database;

  // run the NCurses-based interface?
  bool ncurses_ui;

  // run the line-oriented interface?
  bool line_ui;

  // parallelism (0 == auto)
  unsigned long threads;

  // colour terminal output on or off
  colour_t colour;

  // arguments to pass to Clang when parsing C/C++
  char **cxx_argv;
  size_t cxx_argc;

  // debug mode
  bool debug;

} option_t;

extern option_t option;

// setup option.database_path after option parsing
int set_db_path(void);

// setup option.src after option parsing
int set_src(void);

// deallocate members of option
void clean_up_options(void);
