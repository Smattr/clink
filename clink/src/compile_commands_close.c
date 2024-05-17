#include "compile_commands.h"
#include <assert.h>
#include <clang-c/CXCompilationDatabase.h>
#include <pthread.h>
#include <stddef.h>

void compile_commands_close(compile_commands_t *cc) {

  if (cc == NULL)
    return;

  (void)pthread_mutex_destroy(&cc->lock);

  if (cc->db != NULL)
    clang_CompilationDatabase_dispose(cc->db);

  *cc = (compile_commands_t){0};
}
