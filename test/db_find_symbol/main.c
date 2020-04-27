// basic test of clink_db_find_symbol()

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
      = { .category = CLINK_DEFINITION, .lineno = 42, .colno = 10 };

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

  int r1 = 0;
  do {

    // lookup a symbol that does not exist
    clink_iter_t *it = NULL;
    if ((r1 = clink_db_find_symbol(db, "foobar", &it))) {
      fprintf(stderr, "clink_db_find_symbol: %s\n", strerror(r1));
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

    // lookup a symbol that exists
    clink_iter_t *it = NULL;
    if ((r2 = clink_db_find_symbol(db, "sym-name", &it))) {
      fprintf(stderr, "clink_db_find_symbol: %s\n", strerror(r2));
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

      if (sym->category != CLINK_DEFINITION) {
        fprintf(stderr, "incorrect symbol category: %d != %d\n",
          (int)sym->category, (int)CLINK_DEFINITION);
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

  // close the database
  if (rc == 0)
    clink_db_close(&db);

  // clean up
  (void)unlink(target);
  (void)rmdir(path);

  // confirm that the database was opened correctly
  assert(rc == 0);

  // confirm that lookup of a missing symbol worked as expected
  assert(r1 == 0);

  // confirm that lookup of a symbol that does exist works as expected
  assert(r2 == 0);

  return 0;
}
