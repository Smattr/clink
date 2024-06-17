#include "../../common/compiler.h"
#include "compile_commands.h"
#include "path.h"
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

/// does this look like a source file being passed to the compiler?
static bool is_source_arg(const char *path) {
  assert(path != NULL);

  // does this look like a compiler flag?
  if (path[0] == '-')
    return false;

  // does it look like a source file Clang would understand?
  if (is_asm(path))
    return true;
  if (is_c(path))
    return true;
  if (is_cxx(path))
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

  // For some reason, libclang’s interface to the compilation database will
  // return hits for headers that do _not_ have specific compile commands in the
  // database. This is useful to us. Except that the returned command often does
  // not work. Specifically it uses the inner compiler driver (`cc`) which does
  // not understand the argument separator (`--`) but then also uses the
  // argument separator in the command. When instructing libclang to parse using
  // this command, it then fails. See if we can detect that situation here and
  // drop the `--` and everything following.
  if (is_cc(av[0])) {
    for (size_t i = 1; i < ac; ++i) {
      if (strcmp(av[i], "--") == 0) {
        for (size_t j = i; j < ac; ++j) {
          free(av[j]);
          av[j] = NULL;
        }
        ac = i;
        break;
      }
    }
  }

  // Drop anything that looks like a source being passed to the compiler. We do
  // this rather than more specific matching of `source` itself because the
  // command may reference the source via a relative or symlink-containing path.
  for (size_t i = 1; i < ac;) {
    if (is_source_arg(av[i])) {
      free(av[i]);
      for (size_t j = i; j + 1 < ac; ++j)
        av[j] = av[j + 1];
      av[ac - 1] = NULL;
      --ac;
    } else {
      ++i;
    }
  }

  // If the compiler in use when the compilation database was generated was not
  // Clang, commands may include warning options Clang does not understand (e.g.
  // -Wcast-align=strict). Passing `displayDiagnostics=0` to `clang_createIndex`
  // seems insufficient to silence the resulting -Wunknown-warning-option
  // diagnostics that are then printed to stderr. To work around this, strip
  // anything that looks like a warning option.
  for (size_t i = 1; i < ac;) {
    if (av[i][0] == '-' && av[i][1] == 'W') {
      free(av[i]);
      for (size_t j = i; j + 1 < ac; ++j)
        av[j] = av[j + 1];
      av[ac - 1] = NULL;
      --ac;
    } else {
      ++i;
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
