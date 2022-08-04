#include "../../common/compiler.h"
#include "compile_commands.h"
#include "path.h"
#include <assert.h>
#include <clang-c/CXCompilationDatabase.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int compile_commands_open(compile_commands_t *cc, const char *directory) {

  assert(cc != NULL);
  assert(directory != NULL);

  memset(cc, 0, sizeof(*cc));

  // check whether compile_commands.json exists
  {
    char *compile_commands = NULL;
    int rc = join(directory, "compile_commands.json", &compile_commands);
    if (rc != 0)
      return rc;
    rc = access(compile_commands, R_OK);
    free(compile_commands);
    if (rc != 0)
      return rc;
  }

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
