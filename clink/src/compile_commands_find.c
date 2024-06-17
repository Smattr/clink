#include "../../common/compiler.h"
#include "compile_commands.h"
#include <assert.h>
#include <clang-c/CXCompilationDatabase.h>
#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/// is this the raw compiler driver (as opposed to a front end wrapper)?
static bool is_cc(const char *path) {
  if (strcmp(path, "cc") == 0)
    return true;
  if (strlen(path) >= 3 && strcmp(path + strlen(path) - 3, "/cc") == 0)
    return true;
  if (strcmp(path, "c++") == 0)
    return true;
  if (strlen(path) >= 4 && strcmp(path + strlen(path) - 4, "/c++") == 0)
    return true;
  return false;
}

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

  // how many arguments do we have?
  size_t num_args = clang_CompileCommand_getNumArgs(cmd);
  assert(num_args > 0);
  assert(num_args < SIZE_MAX);

  // extract the arguments with a trailing NULL
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

  // only accept the command if its last parameter is the source itself
  if (strcmp(av[ac - 1], source) != 0) {
    rc = ENOMSG;
    goto done;
  }
  free(av[ac - 1]);
  av[ac - 1] = NULL;
  --ac;

  // For some reason, libclang’s interface to the compilation database will
  // return hits for headers that do _not_ have specific compile commands in the
  // database. This is useful to us. Except that the returned command often does
  // not work. Specifically it uses the inner compiler driver (`cc`) which does
  // not understand the argument separator (`--`) but then also uses the
  // argument separator in the command. When instructing libclang to parse using
  // this command, it then fails. See if we can detect that situation here and
  // drop the `--`.
  if (ac >= 2) {
    do {
      if (!is_cc(av[0]))
        break;

      if (strcmp(av[ac - 1], "--") == 0) {
        free(av[ac - 1]);
        av[ac - 1] = NULL;
        --ac;
      }
    } while (0);
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
