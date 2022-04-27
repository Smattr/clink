#include "../../common/compiler.h"
#include <assert.h>
#include <clink/db.h>
#include <clink/vim.h>
#include <errno.h>
#include <stddef.h>

typedef struct {
  clink_db_t *db;
  const char *filename;
  unsigned long lineno;
} state_t;

static int insert(void *state, const char *line) {
  assert(state != NULL);
  assert(line != NULL);

  state_t *s = state;

  ++s->lineno;
  return clink_db_add_line(s->db, s->filename, s->lineno, line);
}

int clink_vim_read_into(clink_db_t *db, const char *filename) {

  if (UNLIKELY(db == NULL))
    return EINVAL;

  if (UNLIKELY(filename == NULL))
    return EINVAL;

  state_t s = {.db = db, .filename = filename};
  return clink_vim_read(filename, insert, &s);
}
