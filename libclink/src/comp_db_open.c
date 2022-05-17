#include "../../common/compiler.h"
#include "comp_db.h"
#include <assert.h>
#include <clang-c/CXCompilationDatabase.h>
#include <clink/c.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>

/// convert a `CXCompilationDatabase_Error` to an `errno`
static int cxcdb_error_to_errno(int error) {

  // the only error defined is something highly generic
  if (error == CXCompilationDatabase_CanNotLoadDatabase)
    return EIO;

  return 0;
}

int clink_comp_db_open(clink_comp_db_t **db, const char *path) {

  if (UNLIKELY(db == NULL))
    return EINVAL;

  if (UNLIKELY(path == NULL))
    return EINVAL;

  clink_comp_db_t *d = calloc(1, sizeof(*d));
  if (UNLIKELY(d == NULL))
    return ENOMEM;

  int rc = 0;

  // load the compilation database
  CXCompilationDatabase_Error error = CXCompilationDatabase_NoError;
  d->db = clang_CompilationDatabase_fromDirectory(path, &error);
  if (error != CXCompilationDatabase_NoError) {
    rc = cxcdb_error_to_errno(error);
    goto done;
  }

  // success
  *db = d;

done:
  if (rc != 0)
    clink_comp_db_close(db);

  return rc;
}
