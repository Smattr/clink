#include <assert.h>
#include <clink/clink.h>
#include "../../common/compiler.h"
#include <errno.h>
#include <limits.h>
#include "option.h"
#include "path.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

option_t option = {
  .database_path = NULL,
  .src = NULL,
  .src_len = 0,
  .update_database = true,
  .ncurses_ui = true,
  .line_ui = false,
  .threads = 0,
  .colour = AUTO,
  .cxx_argv = NULL,
  .cxx_argc = 0,
  .stdinc = true,
  .debug = false,
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

int set_cxx_flags(char **includes, size_t includes_len) {

  // alias the parameters to names that more clearly fit our usage
  char **user = includes;
  size_t user_len = includes_len;

  int rc = 0;

  // retrieve the default include paths to specify as system directories
  char **system = NULL;
  size_t system_len = 0;
  if (option.stdinc) {
    if (UNLIKELY((rc = clink_compiler_includes(NULL, &system, &system_len))))
      return rc;
  }

  // ensure calculating the allocation below will not overflow
  if (UNLIKELY(SIZE_MAX / 2 < system_len))
    return EOVERFLOW;
  if (UNLIKELY(SIZE_MAX / 2 < user_len))
    return EOVERFLOW;
  if (UNLIKELY(SIZE_MAX - 2 * system_len < 2 * user_len))
    return EOVERFLOW;

  // allocate space for both system and user directories with a prefix
  assert(option.cxx_argv == NULL && "setting up CXX argv twice");
  size_t argc = 2 * system_len + 2 * user_len;
  char **argv = calloc(argc, sizeof(argv[0]));
  if (UNLIKELY(argv == NULL)) {
    rc = ENOMEM;
    goto done;
  }

  // copy in the system paths, taking ownership
  for (size_t i = 0; i < system_len; ++i) {
    argv[i * 2] = strdup("-isystem");
    if (UNLIKELY(argv[i * 2] == NULL)) {
      rc = ENOMEM;
      goto done;
    }
    argv[i * 2 + 1] = system[i];
    system[i] = NULL;
  }

  // copy in the user paths, taking ownership
  for (size_t i = 0; i < user_len; ++i) {
    size_t offset = i + system_len;
    argv[offset * 2] = strdup("-I");
    if (UNLIKELY(argv[offset * 2] == NULL)) {
      rc = ENOMEM;
      goto done;
    }
    argv[offset * 2 + 1] = user[i];
    user[i] = NULL;
  }

done:
  if (rc) {
    for (size_t i = 0; i < argc; ++i)
      free(argv[i]);
    free(argv);
  }
  for (size_t i = 0; i < system_len; ++i)
    free(system[i]);
  free(system);

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

  for (size_t i = 0; i < option.cxx_argc; ++i)
    free(option.cxx_argv[i]);
  free(option.cxx_argv);
  option.cxx_argv = NULL;
  option.cxx_argc = 0;
}
