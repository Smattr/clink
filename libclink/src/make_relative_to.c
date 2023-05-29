#include "make_relative_to.h"
#include "db.h"
#include <assert.h>
#include <stddef.h>
#include <string.h>

const char *make_relative_to(const clink_db_t *db, const char *path) {

  assert(db != NULL);
  assert(path != NULL);

  size_t len = strlen(db->dir);
  if (strncmp(path, db->dir, len) != 0)
    return path;

  return path + len;
}
