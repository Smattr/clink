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

/** setup flags for the C++ compiler
 *
 * This function assumes the caller wants system include directories enabled
 * (as if `-nostdinc` was not supplied). This may change in future.
 *
 * None of the data of the input array’s entries are modified, but the input
 * array’s entries themselves may be modified by taking ownership of their
 * memory and overwriting them with NULL. On success or failure, the caller
 * should assume they can do nothing with this array except free it and all its
 * entries.
 *
 * \param includes An array of paths to be passed to the compiler with the `-I`
 *   option.
 * \param includes_len The length of `includes`.
 * \return 0 on success or an errno on failure.
 */
int set_cxx_flags(char **includes, size_t includes_len);

// deallocate members of option
void clean_up_options(void);
