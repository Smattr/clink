#include "option.h"
#include "path.h"
#include <assert.h>
#include <clink/clink.h>
#include <errno.h>
#include <limits.h>
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
    .debug = false,
    .highlighting = BEHAVIOUR_AUTO,
    .parse_asm = GENERIC,
    .parse_c = CLANG,
    .parse_cxx = CLANG,
    .parse_def = GENERIC,
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

void clean_up_options(void) {

  free(option.database_path);
  option.database_path = NULL;

  for (size_t i = 0; i < option.src_len; ++i)
    free(option.src[i]);
  free(option.src);
  option.src = NULL;
  option.src_len = 0;
}
