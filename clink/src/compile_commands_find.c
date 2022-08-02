#include "../../common/compiler.h"
#include "compile_commands.h"
#include <assert.h>
#include <clang-c/CXCompilationDatabase.h>
#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

int compile_commands_find(compile_commands_t *cc, const char *source,
                          size_t *argc, char ***argv) {

  assert(cc != NULL);
  assert(source != NULL);

  int rc = 0;
  size_t ac = 0;
  char **av = NULL;

  rc = pthread_mutex_lock(&cc->lock);
  if (UNLIKELY(rc != 0))
    return rc;

  // find the compilation commands for this file
  CXCompileCommands cmds =
      clang_CompilationDatabase_getCompileCommands(cc->db, source);

  // do we have any?
  if (clang_CompileCommands_getSize(cmds) == 0) {
    rc = ENOMSG;
    goto done;
  }

  // take the first and ignore the rest
  CXCompileCommand cmd = clang_CompileCommands_getCommand(cmds, 0);

  // extract the arguments, with a trailing NULL
  size_t num_args = clang_CompileCommand_getNumArgs(cmd);
  assert(num_args > 0);
  assert(num_args < SIZE_MAX - 1);
  av = calloc(num_args + 1, sizeof(av[0]));
  if (UNLIKELY(av == NULL)) {
    rc = ENOMEM;
    goto done;
  }
  ac = num_args;
  for (size_t i = 0; i < ac; ++i) {
    CXString arg = clang_CompileCommand_getArg(cmd, (unsigned)i);
    const char *argstr = clang_getCString(arg);
    assert(argstr != NULL);
    av[i] = strdup(argstr);
    clang_disposeString(arg);
    if (UNLIKELY(av[i] == NULL)) {
      rc = ENOMEM;
      goto done;
    }
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

  (void)pthread_mutex_unlock(&cc->lock);

  return rc;
}
