#include "../../common/compiler.h"
#include "compile_commands.h"
#include "debug.h"
#include "path.h"
#include <assert.h>
#include <clang-c/CXCompilationDatabase.h>
#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/// are two strings equal?
static bool streq(const char *a, const char *b) {
  assert(a != NULL);
  assert(b != NULL);
  return strcmp(a, b) == 0;
}

/// does a string start with the given prefix?
static bool startswith(const char *s, const char *prefix) {
  assert(s != NULL);
  assert(prefix != NULL);
  return strncmp(s, prefix, strlen(prefix)) == 0;
}

/// is this the raw compiler driver (as opposed to a front end wrapper)?
static bool is_cc(const char *path) {
  if (streq(path, "cc"))
    return true;
  if (strlen(path) >= 3 && streq(path + strlen(path) - 3, "/cc"))
    return true;
  if (streq(path, "c++"))
    return true;
  if (strlen(path) >= 4 && streq(path + strlen(path) - 4, "/c++"))
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

/// expand relative paths into absolute paths in a compiler option
///
/// The `ai` parameter is advanced over any argument(s) that are acted upon
/// (optionally replaced) by this function.
///
/// \param argv Compiler arguments
/// \param argc Number of elements in `argv`
/// \param ai [inout] Argument to examine
/// \param wd Absolute path to working directory in which this command was run
/// \param try Compiler option to check for
/// \param uses_sysroot Whether to account  `=` and `$SYSROOT` prefixes
/// \return 0 on success or an errno on failure
static int make_absolute(char **argv, size_t argc, size_t *ai, const char *wd,
                         const char *try, bool uses_sysroot) {
  assert(argv != NULL || argc == 0);
  assert(*ai < argc);
  assert(wd != NULL);
  assert(!is_relative(wd));
  assert(try != NULL);

  // does this look like `-I a/path/arg`?
  if (streq(argv[*ai], try)) {

    // are we out of arguments?
    if (*ai + 1 == argc)
      return 0;
    ++*ai;

    do {

      // Quoting GCC docs, “If _dir_ begins with ‘=’ or `$SYSROOT`, then the ‘=’
      // or `$SYSROOT` is replaced by the sysroot prefix”
      if (uses_sysroot) {
        if (startswith(argv[*ai], "=") || startswith(argv[*ai], "$SYSROOT"))
          break;
      }

      // we only need to replace relative paths
      if (!is_relative(argv[*ai]))
        break;

      // construct an absolute path
      char *replacement = NULL;
      const int rc = join(wd, argv[*ai], &replacement);
      if (ERROR(rc))
        return rc;

      DEBUG("replacing compiler option %s with %s", argv[*ai], replacement);
      free(argv[*ai]);
      argv[*ai] = replacement;
    } while (0);

    ++*ai;
    return 0;
  }

  // if it is a getopt-style long argument, the alternative form will be
  // `--foo=BAR` rather than `-fooBAR`
  const bool needs_equals = startswith(try, "--");

  // does this look like `-Ia/path/arg`?
  if (startswith(argv[*ai], try) &&
      (!needs_equals || argv[*ai][strlen(try)] == '=')) {
    const char *stem = argv[*ai] + strlen(try);
    if (needs_equals)
      ++stem;
    do {

      if (uses_sysroot) {
        if (startswith(stem, "=") || startswith(stem, "$SYSROOT"))
          break;
      }

      if (!is_relative(stem))
        break;

      char *suffix = NULL;
      const int rc = join(wd, stem, &suffix);
      if (ERROR(rc))
        return rc;

      const size_t replacement_size =
          strlen(try) + (needs_equals ? 1 : 0) + strlen(suffix) + 1;
      char *replacement = malloc(replacement_size);
      if (ERROR(replacement == NULL)) {
        free(suffix);
        return ENOMEM;
      }

      snprintf(replacement, replacement_size, "%s%s%s", try,
               needs_equals ? "=" : "", suffix);
      free(suffix);
      DEBUG("replacing compiler option %s with %s", argv[*ai], replacement);
      free(argv[*ai]);
      argv[*ai] = replacement;

    } while (0);

    ++*ai;
    return 0;
  }

  return 0;
}

int compile_commands_find(compile_commands_t *cc, const char *source,
                          size_t *argc, char ***argv) {

  assert(cc != NULL);
  assert(source != NULL);

  int rc = 0;
  size_t ac = 0;
  char **av = NULL;
  CXString working;
  const char *wd = NULL;

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

  // extract the working directory this command was run from
  working = clang_CompileCommand_getDirectory(cmd);
  wd = clang_getCString(working);
  assert(wd != NULL);

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
      if (streq(av[i], "--")) {
        for (size_t j = i; j < ac; ++j) {
          DEBUG("dropping compiler option %s", av[j]);
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
      DEBUG("dropping compiler option %s", av[i]);
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
    if (av[i][0] == '-' && av[i][1] == 'W' &&
        (av[i][2] == '\0' || av[i][3] != ',')) {
      DEBUG("dropping compiler option %s", av[i]);
      free(av[i]);
      for (size_t j = i; j + 1 < ac; ++j)
        av[j] = av[j + 1];
      av[ac - 1] = NULL;
      --ac;
    } else {
      ++i;
    }
  }

  // Scan for #include-like options that may contain relative paths. We need to
  // expand these into absolute paths in order to avoid either (a) relying on
  // the current working directory or (b) chdir-ing into the directory this
  // command was run from. We do not want to do (b) even if it would work to
  // avoid thinking about thread safety and whether or not the working directory
  // still exists or is accessible.
  for (size_t i = 1; i < ac;) {
    const size_t saved_i = i;

    // the documented GCC options that take a relative directory we should remap
    const struct {
      const char *option; ///< the option text
      bool uses_sysroot; ///< whether to account for `=` and `$SYSROOT` prefixes
    } dir_options[] = {
        {"-I", true},         {"-iquote", true},    {"-isystem", true},
        {"-idirafter", true}, {"-isysroot", false}, {"-imultilib", false},
        {"--sysroot", false},
    };

    for (size_t j = 0; j < sizeof(dir_options) / sizeof(dir_options[0]); ++j) {
      // detect option and make any paths it refers to absolute
      if (ERROR((rc = make_absolute(av, ac, &i, wd, dir_options[j].option,
                                    dir_options[j].uses_sysroot))))
        goto done;
      // did this consume the current argument?
      if (saved_i != i)
        break;
    }

    if (saved_i == i)
      ++i;
  }

  // success
  *argc = ac;
  *argv = av;
  ac = 0;
  av = NULL;

done:
  if (wd != NULL)
    clang_disposeString(working);

  for (size_t i = 0; av != NULL && i < ac; ++i)
    free(av[i]);
  free(av);

  clang_CompileCommands_dispose(cmds);

  (void)pthread_mutex_unlock(&cc->lock);

  return rc;
}
