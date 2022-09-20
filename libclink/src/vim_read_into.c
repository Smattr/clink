#include "debug.h"
#include <assert.h>
#include <clink/db.h>
#include <clink/vim.h>
#include <ctype.h>
#include <errno.h>
#include <stddef.h>
#include <string.h>

typedef struct {
  clink_db_t *db;
  const char *filename;
  unsigned long lineno;
} state_t;

static int insert(void *state, char *line) {
  assert(state != NULL);
  assert(line != NULL);

  state_t *s = state;

  // eagerly strip leading white space from the line because we know the UI
  // wants it stripped eventually
  for (char *p = line; *p != '\0';) {
    if (isspace(*p)) {
      memmove(p, p + 1, strlen(p));
    } else if (*p == '\033') { // CSI
      // skip over it to the terminator
      for (char *q = p + 1; *q != '\0'; ++q) {
        if (*q == 'm') {
          p = q;
          break;
        }
      }
      ++p;
    } else { // non-white-space content
      break;
    }
  }

  ++s->lineno;
  return clink_db_add_line(s->db, s->filename, s->lineno, line);
}

int clink_vim_read_into(clink_db_t *db, const char *filename) {

  if (ERROR(db == NULL))
    return EINVAL;

  if (ERROR(filename == NULL))
    return EINVAL;

  state_t s = {.db = db, .filename = filename};
  return clink_vim_read(filename, insert, &s);
}
