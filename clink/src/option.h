#pragma once

#include "compile_commands.h"
#include <stdbool.h>
#include <stddef.h>

typedef enum {
  AUTO,
  ALWAYS,
  NEVER,
} colour_t;

typedef enum {
  BEHAVIOUR_AUTO, ///< pick `LAZY` or `EAGER` based on amount of work
  LAZY,           ///< do action on-demand, when its results are needed
  EAGER,          ///< do action upfront
} behaviour_t;

typedef enum {
  CLANG,   ///< libclang-based parser
  GENERIC, ///< generic parser
  OFF,     ///< skip parsing
} parser_t;

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

  // parallelism (0 == auto)
  unsigned long threads;

  // colour terminal output on or off
  colour_t colour;

  // debug mode
  bool debug;

  // which strategy to apply to syntax highlighting
  behaviour_t highlighting;

  // how to parse each file type
  parser_t parse_asm;
  parser_t parse_c;
  parser_t parse_cxx;
  parser_t parse_def;
  parser_t parse_python;

  // arguments to pass to Clang
  size_t clang_argc;
  char **clang_argv;

  // compile_commands.json database
  compile_commands_t compile_commands;

} option_t;

extern option_t option;

// setup option.database_path after option parsing
int set_db_path(void);

// setup option.compile_commands after option parsing
int set_compile_commands(void);

// setup option.src after option parsing
int set_src(void);

/** setup flags for Clang
 *
 * This function assumes the caller wants system include directories enabled.
 *
 * \return 0 on success or an errno on failure.
 */
int set_clang_flags(void);

// deallocate members of option
void clean_up_options(void);
