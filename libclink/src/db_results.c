#include <assert.h>
#include <clink/db.h>
#include <clink/symbol.h>
#include <errno.h>
#include <stdlib.h>

typedef struct {
  struct clink_result *data;
  size_t size;
  size_t capacity;
} results_t;

static int append_result(const struct clink_result *result, void *rs_void) {

  results_t *rs = rs_void;

  int rc = 0;

  // do we need to expand the result collection?
  if (rs->size == rs->capacity) {
    size_t capacity = rs->capacity == 0 ? 1 : rs->capacity * 2;
    struct clink_result *d = realloc(rs->data, capacity * sizeof(d[0]));
    if (d == NULL)
      return ENOMEM;
    rs->data = d;
    rs->capacity = capacity;
  }

  if ((rc = clink_result_copy(&rs->data[rs->size], result)))
    return rc;

  rs->size++;
  return 0;
}

static void clear_results(results_t *rs) {
  for (size_t i = 0; i < rs->size; i++)
    clink_result_clear(&rs->data[i]);
}

int clink_db_results(struct clink_db *db, const char *name,
    int (*finder)(struct clink_db *db, const char *name,
      int (*callback)(const struct clink_result *result, void *state),
      void *callback_state),
    struct clink_result **results, size_t *results_size) {

  assert(db != NULL);
  assert(name != NULL);
  assert(finder != NULL);
  assert(results != NULL);
  assert(results_size != NULL);

  results_t rs = { 0 };
  int rc = 0;
  
  if ((rc = finder(db, name, append_result, &rs)))
    goto done;

done:
  if (rc != 0) {
    clear_results(&rs);
  } else {
    *results = rs.data;
    *results_size = rs.size;
  }

  return rc;
}
