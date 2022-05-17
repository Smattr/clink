#include "comp_db.h"
#include <clang-c/CXCompilationDatabase.h>
#include <clink/c.h>
#include <stdlib.h>

void clink_comp_db_close(clink_comp_db_t **db) {

  if (db == NULL)
    return;

  if (*db == NULL)
    return;

  if ((*db)->db != NULL)
    clang_CompilationDatabase_dispose((*db)->db);
  (*db)->db = NULL;

  free(*db);
  *db = NULL;
}
