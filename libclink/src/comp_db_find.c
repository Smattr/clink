#include "../../common/compiler.h"
#include "comp_db.h"
#include "debug.h"
#include <assert.h>
#include <clang-c/CXCompilationDatabase.h>
#include <clang-c/CXString.h>
#include <clink/c.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

int clink_comp_db_find(const clink_comp_db_t *db, const char *path,
                       size_t *argc, char ***argv) {

  if (UNLIKELY(db == NULL))
    return EINVAL;

  if (UNLIKELY(path == NULL))
    return EINVAL;

  if (UNLIKELY(argc == NULL))
    return EINVAL;

  if (UNLIKELY(argv == NULL))
    return EINVAL;

  // find all the compile commands for this source
  CXCompileCommands cmds =
      clang_CompilationDatabase_getCompileCommands(db->db, path);
  if (cmds == NULL) {
    DEBUG("no compile command found for %s", path);
    return ENOMSG;
  }

  int rc = 0;
  size_t ac = 0;
  char **av = NULL;

  // we do not support more than one alternative
  if (clang_CompileCommands_getSize(cmds) != 1) {
    DEBUG("multiple compile commands found for %s", path);
    rc = ENOMSG;
    goto done;
  }

  // extract the (now-known-unique) command
  CXCompileCommand cmd = clang_CompileCommands_getCommand(cmds, 0);
  assert(cmd != NULL);

  // TODO: canonicalize/absolutize paths?

  // setup an array to receive all the arguments of this command
  {
    unsigned num_args = clang_CompileCommand_getNumArgs(cmd);

    // if this command does not include at least the compiler executable and the
    // source path itself, treat this as a lookup miss
    if (num_args < 2) {
      DEBUG("suppressing <2 argument command in compilation database");
      return ENOMSG;
    }
    num_args -= 2;

    av = calloc(num_args, sizeof(av[0]));
    if (UNLIKELY(num_args > 0 && av == NULL)) {
      rc = ENOMEM;
      goto done;
    }
    ac = num_args;
  }

  // translate the arguments into C strings
  size_t source_seen = 0;
  for (size_t src = 1, dst = 0; src < ac + 2; ++src) {

    CXString cxs = clang_CompileCommand_getArg(cmd, (unsigned)src);
    const char *cstr = clang_getCString(cxs);

    // skip if this is the source path itself
    if (strcmp(cstr, path) == 0) {
      ++source_seen;
      clang_disposeString(cxs);
      continue;
    }

    av[dst] = strdup(cstr);
    clang_disposeString(cxs);

    if (UNLIKELY(av[dst] == NULL)) {
      rc = ENOMEM;
      goto done;
    }

    ++dst;
  }

  // if we saw either did not see the source path or saw multiple copies of it,
  // we do not know exactly how to interpret this command so consider it a miss
  if (UNLIKELY(source_seen != 1)) {
    DEBUG("suppressing command with %zu copies of source path %s", source_seen,
          path);
    rc = ENOMSG;
    goto done;
  }

  // success
  *argc = ac;
  *argv = av;
  ac = 0;
  av = NULL;

done:
  for (size_t i = 0; i < ac; ++i)
    free(av[i]);
  free(av);
  clang_CompileCommands_dispose(cmds);

  return rc;
}
