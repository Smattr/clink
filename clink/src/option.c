#include "option.h"
#include "../../common/compiler.h"
#include "compile_commands.h"
#include "debug.h"
#include "path.h"
#include <assert.h>
#include <clink/clink.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

option_t option = {
    .database_path = NULL,
    .src = NULL,
    .src_len = 0,
    .update_database = true,
    .ui = true,
    .threads = 0,
    .colour = AUTO,
    .animation = true,
    .debug = false,
    .highlighting = BEHAVIOUR_AUTO,
    .parse_asm = GENERIC,
    .parse_c = PARSER_AUTO,
    .parse_cxx = PARSER_AUTO,
    .parse_def = GENERIC,
    .parse_lex = PARSER_AUTO,
    .parse_python = GENERIC,
    .parse_tablegen = GENERIC,
    .parse_yacc = PARSER_AUTO,
    .clang_argc = 0,
    .clang_argv = NULL,
    .compile_commands = {0},
    .script = NULL,
};

int set_db_path(void) {

  // if the user set the database path, no need to do anything else
  if (option.database_path != NULL)
    return 0;

  int rc = 0;

  // get our current directory
  char *wd = NULL;
  if ((rc = cwd(&wd)))
    return rc;

  // start at this current directory
  char *branch = strdup(wd);
  if (branch == NULL) {
    rc = ENOMEM;
    goto done;
  }

  while (true) {

    // create a path to a database at this level
    char *candidate = NULL;
    if ((rc = join(branch, ".clink.db", &candidate)))
      goto done;

    // if this exists, we are done
    if (access(candidate, F_OK) == 0) {
      option.database_path = candidate;
      goto done;
    }

    // is this the root of a repository checkout?
    // TODO: support Git Worktrees?
    static const char *VCS_DIR[] = {".git", ".hg", ".svn"};
    for (size_t i = 0; i < sizeof(VCS_DIR) / sizeof(VCS_DIR[0]); i++) {

      // derive the candidate repository metadata dir
      char *vcs_dir = NULL;
      if ((rc = join(branch, VCS_DIR[i], &vcs_dir))) {
        free(candidate);
        goto done;
      }

      // if it exists, use an adjacent database
      if (access(vcs_dir, F_OK) == 0) {
        free(vcs_dir);
        option.database_path = candidate;
        goto done;
      }

      free(vcs_dir);
    }

    free(candidate);

    // if we just checked the file system root, give up
    if (is_root(branch))
      break;

    // otherwise move one directory up and try again
    char *next = NULL;
    rc = dirname(branch, &next);
    free(branch);
    branch = next;
    if (rc)
      goto done;
  }

  // if we still did not find a database, default to the current directory
  if ((rc = join(wd, ".clink.db", &option.database_path)))
    goto done;

done:
  free(branch);
  free(wd);

  return rc;
}

int set_src(void) {

  // if we were given some explicit sources, we need nothing further
  if (option.src_len > 0)
    return 0;

  int rc = 0;

  // otherwise, prepare to add a single directory
  assert(option.src == NULL && "logic bug in set_src()");
  option.src = calloc(1, sizeof(option.src[0]));
  if (option.src == NULL)
    return ENOMEM;
  option.src_len = 1;

  // use the directory the database lives in as a source
  assert(option.database_path != NULL && "no database set");
  if ((rc = dirname(option.database_path, &option.src[0])))
    goto done;

done:
  if (rc) {
    free(option.src);
    option.src = NULL;
    option.src_len = 0;
  }

  return rc;
}

int set_clang_flags(void) {

  int rc = 0;

  // retrieve the default include paths to specify as system directories
  char **system = NULL;
  size_t system_len = 0;
  if (UNLIKELY((rc = clink_compiler_includes(NULL, &system, &system_len))))
    return rc;

  // ensure calculating the allocation below will not overflow
  if (UNLIKELY(SIZE_MAX / 2 - 2 < system_len))
    return EOVERFLOW;

  // allocate space for the system directories with a prefix
  assert(option.clang_argv == NULL && "setting up Clang arguments twice");
  size_t argc = 2 * system_len + 1;
  char **argv = calloc(argc + 1, sizeof(argv[0]));
  if (UNLIKELY(argv == NULL)) {
    rc = ENOMEM;
    goto done;
  }

  // name the compiler itself to satisfy libclang
  argv[0] = strdup("clang");
  if (UNLIKELY(argv[0] == NULL)) {
    rc = ENOMEM;
    goto done;
  }

  // copy in the system paths, taking ownership
  for (size_t i = 0; i < system_len; ++i) {
    argv[1 + i * 2] = strdup("-isystem");
    if (UNLIKELY(argv[1 + i * 2] == NULL)) {
      rc = ENOMEM;
      goto done;
    }
    argv[1 + i * 2 + 1] = system[i];
    system[i] = NULL;
  }

done:
  if (rc) {
    for (size_t i = 0; argv != NULL && i < argc; ++i)
      free(argv[i]);
    free(argv);
  } else {
    option.clang_argc = argc;
    option.clang_argv = argv;
  }
  for (size_t i = 0; i < system_len; ++i)
    free(system[i]);
  free(system);

  return rc;
}

int set_compile_commands(void) {

  assert(option.database_path != NULL);

  // if the user has manually set compile command, we are done
  if (option.compile_commands.db != NULL)
    return 0;

  int rc = 0;
  char *dir = NULL;
  char *joined = NULL;

  // obtain the directory containing our database
  rc = dirname(option.database_path, &dir);
  if (rc != 0)
    goto done;

  // see if there is a usable compile_commands.json here
  {
    int r = compile_commands_open(&option.compile_commands, dir);
    if (r == 0) {
      DEBUG("using %s/compile_commands.json compilation database", dir);
      goto done;
    }
    DEBUG("failed to open %s/compile_commands.json", dir);
  }

  // try a build subdirectory
  rc = join(dir, "build", &joined);
  if (rc != 0)
    goto done;
  {
    int r = compile_commands_open(&option.compile_commands, joined);
    if (r == 0) {
      DEBUG("using %s/compile_commands.json compilation database", joined);
      goto done;
    }
    DEBUG("failed to open %s/compile_commands.json", joined);
  }

  // nothing usable found
  rc = ENOMSG;

done:
  free(joined);
  free(dir);

  return rc;
}

void clean_up_options(void) {

  free(option.database_path);
  option.database_path = NULL;

  for (size_t i = 0; i < option.src_len; ++i)
    free(option.src[i]);
  free(option.src);
  option.src = NULL;
  option.src_len = 0;

  for (size_t i = 0; i < option.clang_argc; ++i)
    free(option.clang_argv[i]);
  free(option.clang_argv);
  option.clang_argv = NULL;
  option.clang_argc = 0;

  if (option.compile_commands.db != 0)
    compile_commands_close(&option.compile_commands);

  free(option.script);
  option.script = NULL;
}
