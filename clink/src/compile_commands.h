/// \file
/// \brief therad-safe interface to libclang’s compilation database API

#pragma once

#include <clang-c/CXCompilationDatabase.h>
#include <pthread.h>
#include <stddef.h>

/// a handle to a compilation database, as used by `clangd`
typedef struct {
  CXCompilationDatabase db; ///< underlying compile_commands.json
  pthread_mutex_t lock;     ///< guard to use when reading
} compile_commands_t;

/** create a handle to a compilation database
 *
 * The underlying Libclang API does not provide details when accessing a
 * database fails, so `EIO` is returned in these circumstances. That is, the
 * caller receives the same error for compile_commands.json not found, malformed
 * JSON, out of memory, …
 *
 * \param cc [out] Created handle on success
 * \param directory Path to a directory containing a compile_commands.json
 * \returns 0 on success or an errno on failure
 */
int compile_commands_open(compile_commands_t *cc, const char *directory);

/** find the compilation command for a given source file
 *
 * The input file, `source`, is expected to be an absolute path. If there are
 * multiple compilation commands for this file (which can happen in a
 * compile_commands.json) only the first is returned.
 *
 * This function is thread-safe. Multiple threads can call into it concurrently
 * and their lookups will be serialised.
 *
 * \param cc Compilation database to lookup
 * \param source Path of source file to search for
 * \param argc [out] Length of `argv` on success
 * \param argv [out] Compiler command line on success
 * \param 0 on success, `ENOMSG` if the database contained no compilation
 *   command for this source, or another errno on failure.
 */
int compile_commands_find(compile_commands_t *cc, const char *source,
                          size_t *argc, char ***argv);

/** destroy a handle to a compilation database
 *
 * It is the caller’s responsibility to ensure that no other threads are using
 * the handle when this destructor is called.
 *
 * \param cc Handle to destroy
 */
void compile_commands_close(compile_commands_t *cc);
