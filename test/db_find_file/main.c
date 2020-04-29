// basic test of clink_db_find_file()

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

  // add another new symbol in a different file
  if (rc == 0) {

    clink_symbol_t symbol
      = { .category = CLINK_DEFINITION, .lineno = 42, .colno = 10 };

    symbol.name = strdup("sym-name2");
    assert(symbol.name != NULL);

    symbol.path = strdup("/foo/bar2");
    assert(symbol.path != NULL);

    symbol.parent = strdup("sym-parent");
    assert(symbol.parent != NULL);

    rc = clink_db_add_symbol(db, &symbol);
    if (rc)
      fprintf(stderr, "clink_db_add_symbol: %s\n", strerror(rc));
  }

  // add another with a common suffix
  if (rc == 0) {

    clink_symbol_t symbol
      = { .category = CLINK_DEFINITION, .lineno = 42, .colno = 10 };

    symbol.name = strdup("sym-name3");
    assert(symbol.name != NULL);

    symbol.path = strdup("/baz/bar");
    assert(symbol.path != NULL);

    symbol.parent = strdup("sym-parent3");
    assert(symbol.parent != NULL);

    rc = clink_db_add_symbol(db, &symbol);
    if (rc)
      fprintf(stderr, "clink_db_add_symbol: %s\n", strerror(rc));
  }

  int r1 = 0;
  do {

    // lookup a file that does not exist in the database
    clink_iter_t *it = NULL;
    if ((r1 = clink_db_find_file(db, "/foo/qux", &it))) {
      fprintf(stderr, "clink_db_find_file: %s\n", strerror(r1));
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

    // lookup a file that does exist
    clink_iter_t *it = NULL;
    if ((r2 = clink_db_find_file(db, "/foo/bar2", &it))) {
      fprintf(stderr, "clink_db_find_file: %s\n", strerror(r2));
      break;
    }

    // confirm this iterator finds something
    if (!clink_iter_has_next(it)) {
      fprintf(stderr, "iterator unexpectedly is empty\n");
      r2 = -1;

    } else {
      const char *p = NULL;
      if ((r2 = clink_iter_next_str(it, &p))) {
        fprintf(stderr, "clink_iter_next_str: %s\n", strerror(r2));
        break;
      }

      if (strcmp(p, "/foo/bar2") != 0) {
        fprintf(stderr, "incorrect file name: %s != %s\n", p, "/foo/bar2");
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

    // lookup a file that should match multiple different paths
    clink_iter_t *it = NULL;
    if ((r3 = clink_db_find_file(db, "bar", &it))) {
      fprintf(stderr, "clink_db_find_file: %s\n", strerror(r3));
      break;
    }

    // we now need to confirm we find both we are expecting
    bool found_foo = false;
    bool found_baz = false;

    if (!clink_iter_has_next(it)) {
      r3 = -1;
      fprintf(stderr, "iterator unexpectedly is empty\n");

    } else {
      const char *p = NULL;
      if ((r3 = clink_iter_next_str(it, &p))) {
        fprintf(stderr, "clink_iter_next_str: %s\n", strerror(r3));
        break;
      }

      if (strcmp(p, "/foo/bar") == 0) {
        found_foo = true;
      } else if (strcmp(p, "/baz/bar") == 0) {
        found_baz = true;
      } else {
        fprintf(stderr, "found unexpected path: %s\n", p);
        r3 = -1;
      }
    }

    if (r3 == 0) {
      if (!clink_iter_has_next(it)) {
        r3 = -1;
        fprintf(stderr, "iterator unexpectedly is empty\n");

      } else {
        const char *p = NULL;
        if ((r3 = clink_iter_next_str(it, &p))) {
          fprintf(stderr, "clink_iter_next_str: %s\n", strerror(r3));
          break;
        }

        if (!found_foo && strcmp(p, "/foo/bar") == 0) {
          found_foo = true;
        } else if (!found_baz && strcmp(p, "/baz/bar") == 0) {
          found_baz = true;
        } else {
          fprintf(stderr, "found unexpected path: %s\n", p);
          r3 = -1;
        }
      }
    }

    if (r3 == 0) {
      if (clink_iter_has_next(it)) {
        r3 = -1;
        fprintf(stderr, "iterator unexpectedly is non-empty\n");
      }
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

  // confirm that lookup of a file that does not exist worked as expected
  assert(r1 == 0);

  // confirm that lookup of a file that does exist works as expected
  assert(r2 == 0);

  // confirm that lookup of multiple files worked as expected
  assert(r3 == 0);

  return 0;
}
