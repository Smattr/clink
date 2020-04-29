// basic test of clink_db_find_call()

// force assertions on
#ifdef NDEBUG
  #undef NDEBUG
#endif

#include <assert.h>
#include <clink/db.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(void) {

  // find where we should be creating temporary files
  const char *tmp = getenv("TMPDIR");
  if (tmp == NULL)
    tmp = "/tmp";

  // construct a temporary path template
  char *path = NULL;
  {
    int r = asprintf(&path, "%s/tmp.XXXXXX", tmp);
    assert(r >= 0);
  }

  // create a temporary directory to work in
  {
    char *r = mkdtemp(path);
    assert(r != NULL);
  }

  // construct a path within the temporary directory
  char *target = NULL;
  {
    int r = asprintf(&target, "%s/target", path);
    assert(r >= 0);
  }

  // open it as a database
  clink_db_t *db = NULL;
  int rc = clink_db_open(&db, target);
  if (rc)
    fprintf(stderr, "clink_db_open: %s\n", strerror(rc));

  // add a new symbol
  if (rc == 0) {

    clink_symbol_t symbol
      = { .category = CLINK_FUNCTION_CALL, .lineno = 42, .colno = 10 };

    symbol.name = strdup("sym-name");
    assert(symbol.name != NULL);

    symbol.path = strdup("/foo/bar");
    assert(symbol.path != NULL);

    symbol.parent = strdup("sym-parent");
    assert(symbol.parent != NULL);

    rc = clink_db_add_symbol(db, &symbol);
    if (rc)
      fprintf(stderr, "clink_db_add_symbol: %s\n", strerror(rc));
  }

  // add another new symbol that is not a call
  if (rc == 0) {

    clink_symbol_t symbol
      = { .category = CLINK_DEFINITION, .lineno = 42, .colno = 10 };

    symbol.name = strdup("sym-name2");
    assert(symbol.name != NULL);

    symbol.path = strdup("/foo/bar");
    assert(symbol.path != NULL);

    symbol.parent = strdup("sym-parent");
    assert(symbol.parent != NULL);

    rc = clink_db_add_symbol(db, &symbol);
    if (rc)
      fprintf(stderr, "clink_db_add_symbol: %s\n", strerror(rc));
  }

  // add another that is not a call with a different parent
  if (rc == 0) {

    clink_symbol_t symbol
      = { .category = CLINK_DEFINITION, .lineno = 42, .colno = 10 };

    symbol.name = strdup("sym-name3");
    assert(symbol.name != NULL);

    symbol.path = strdup("/foo/bar");
    assert(symbol.path != NULL);

    symbol.parent = strdup("sym-parent3");
    assert(symbol.parent != NULL);

    rc = clink_db_add_symbol(db, &symbol);
    if (rc)
      fprintf(stderr, "clink_db_add_symbol: %s\n", strerror(rc));
  }

  int r1 = 0;
  do {

    // lookup calls in a function that does not exist
    clink_iter_t *it = NULL;
    if ((r1 = clink_db_find_call(db, "sym-parent2", &it))) {
      fprintf(stderr, "clink_db_find_call: %s\n", strerror(r1));
      break;
    }

    // confirm this iterator finds nothing
    if (clink_iter_has_next(it)) {
      r1 = -1;
      fprintf(stderr, "iterator unexpectedly is non-empty\n");
    }

    clink_iter_free(&it);

  } while (0);

  int r2 = 0;
  do {

    // lookup a call in a function that does exist
    clink_iter_t *it = NULL;
    if ((r2 = clink_db_find_call(db, "sym-parent", &it))) {
      fprintf(stderr, "clink_db_find_call: %s\n", strerror(r2));
      break;
    }

    // confirm this iterator finds something
    if (!clink_iter_has_next(it)) {
      fprintf(stderr, "iterator unexpectedly is empty\n");
      r2 = -1;

    } else {
      const clink_symbol_t *sym = NULL;
      if ((r2 = clink_iter_next_symbol(it, &sym))) {
        fprintf(stderr, "clink_iter_next_symbol: %s\n", strerror(r2));
        break;
      }

      if (sym->category != CLINK_FUNCTION_CALL) {
        fprintf(stderr, "incorrect symbol category: %d != %d\n",
          (int)sym->category, (int)CLINK_FUNCTION_CALL);
        r2 = -1;
      }

      if (strcmp(sym->name, "sym-name") != 0) {
        fprintf(stderr, "incorrect symbol name: \"%s\" != \"sym-name\"\n",
          sym->name);
        r2 = -1;
      }

      if (sym->lineno != 42) {
        fprintf(stderr, "incorrect symbol line number: %lu != 42\n",
          sym->lineno);
        r2 = -1;
      }

      if (sym->colno != 10) {
        fprintf(stderr, "incorrect symbol column number: %lu != 10\n",
          sym->colno);
        r2 = -1;
      }

      if (strcmp(sym->path, "/foo/bar") != 0) {
        fprintf(stderr, "incorrect symbol path: \"%s\" != \"/foo/bar\"\n",
          sym->path);
        r2 = -1;
      }

      if (strcmp(sym->parent, "sym-parent") != 0) {
        fprintf(stderr, "incorrect symbol parent: \"%s\" != \"sym-parent\"\n",
          sym->parent);
        r2 = -1;
      }

    }

    // confirm that the iterator is now empty
    if (clink_iter_has_next(it)) {
      fprintf(stderr, "iterator unexpectedly not empty\n");
      r2 = -1;
    }

    clink_iter_free(&it);
  } while (0);

  int r3 = 0;
  do {

    // lookup calls within a function that exists but contains no calls
    clink_iter_t *it = NULL;
    if ((r3 = clink_db_find_call(db, "sym-parent3", &it))) {
      fprintf(stderr, "clink_db_find_call: %s\n", strerror(r3));
      break;
    }

    // confirm this iterator finds nothing
    if (clink_iter_has_next(it)) {
      r3 = -1;
      fprintf(stderr, "iterator unexpectedly is non-empty\n");
    }

    clink_iter_free(&it);

  } while (0);

  // close the database
  if (rc == 0)
    clink_db_close(&db);

  // clean up
  (void)unlink(target);
  (void)rmdir(path);

  // confirm that the database was opened correctly
  assert(rc == 0);

  // confirm that lookup of a missing call worked as expected
  assert(r1 == 0);

  // confirm that lookup of a call that does exist works as expected
  assert(r2 == 0);

  // confirm that looking up calls in a function without calls worked as
  // expected
  assert(r3 == 0);

  return 0;
}
