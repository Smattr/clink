#include "../../common/compiler.h"
#include "compile_commands.h"
#include <assert.h>
#include <clang-c/CXCompilationDatabase.h>
#include <errno.h>
#include <pthread.h>
#include <stddef.h>
#include <string.h>

int compile_commands_open(compile_commands_t *cc, const char *directory) {

  assert(cc != NULL);
  assert(directory != NULL);

  memset(cc, 0, sizeof(*cc));

  CXCompilationDatabase_Error err;
  cc->db = clang_CompilationDatabase_fromDirectory(directory, &err);
  if (err != CXCompilationDatabase_NoError)
    return EIO;
  assert(cc->db != NULL);

  int rc = pthread_mutex_init(&cc->lock, NULL);
  if (UNLIKELY(rc != 0)) {
    clang_CompilationDatabase_dispose(cc->db);
    cc->db = NULL;
    return rc;
  }

  return 0;
}
