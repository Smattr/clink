#include "test.h"
#include <assert.h>
#include <clink/clink.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

static const char PATH[] = "include/clink/clink.h";

static int find(clink_db_t *db, const char *path) {

  assert(db != NULL);
  assert(path != NULL);

  int rc = 0;

  clink_iter_t *it = NULL;
  if ((rc = clink_db_find_includer(db, path, &it))) {
    fprintf(stderr, "clink_db_find_includer: %s\n", strerror(rc));
    return -1;
  }

  // confirm this iterator finds the expected record
  {
    const clink_symbol_t *sym = NULL;
    if ((rc = clink_iter_next_symbol(it, &sym))) {
      fprintf(stderr, "clink_iter_next_symbol: %s\n", strerror(rc));
      return -1;
    }

    if (sym->category != CLINK_INCLUDE) {
      fprintf(stderr, "incorrect symbol category: %d != %d\n",
              (int)sym->category, (int)CLINK_INCLUDE);
      return -1;
    }

    if (strcmp(sym->name, PATH) != 0) {
      fprintf(stderr, "incorrect symbol name: \"%s\" != \"%s\"\n", sym->name,
              PATH);
      return -1;
    }

    if (sym->lineno != 42) {
      fprintf(stderr, "incorrect symbol line number: %lu != 42\n", sym->lineno);
      return -1;
    }

    if (sym->colno != 10) {
      fprintf(stderr, "incorrect symbol column number: %lu != 10\n",
              sym->colno);
      return -1;
    }

    if (strcmp(sym->path, "/foo/bar") != 0) {
      fprintf(stderr, "incorrect symbol path: \"%s\" != \"/foo/bar\"\n",
              sym->path);
      return -1;
    }

    if (strcmp(sym->parent, "sym-parent") != 0) {
      fprintf(stderr, "incorrect symbol parent: \"%s\" != \"sym-parent\"\n",
              sym->parent);
      return -1;
    }
  }

  // confirm that the iterator is now empty
  {
    const clink_symbol_t *sym = NULL;
    int r = clink_iter_next_symbol(it, &sym);
    if (r == 0) {
      fprintf(stderr, "iterator unexpectedly is non-empty\n");
      return -1;
    } else if (r != ENOMSG) {
      fprintf(stderr, "clink_iter_next_symbol: %s\n", strerror(r));
      return -1;
    }
  }

  clink_iter_free(&it);

  return 0;
}

TEST("test looking up an included file by its final component works") {

  // construct a unique path
  char *target = mkpath();

  // open it as a database
  clink_db_t *db = NULL;
  int rc = clink_db_open(&db, target);
  if (rc)
    fprintf(stderr, "clink_db_open: %s\n", strerror(rc));

  // add an include that is a path with multiple components
  if (rc == 0) {

    clink_symbol_t symbol = {
        .category = CLINK_INCLUDE, .lineno = 42, .colno = 10};

    symbol.name = (char *)"include/clink/clink.h";
    symbol.path = (char *)"/foo/bar";
    symbol.parent = (char *)"sym-parent";

    rc = clink_db_add_symbol(db, &symbol);
    if (rc)
      fprintf(stderr, "clink_db_add_symbol: %s\n", strerror(rc));
  }

  // lookup the include by full path
  int r1 = find(db, "include/clink/clink.h");

  // lookup the include by stem
  int r2 = find(db, "clink.h");

  // lookup the include by an intermediate suffix
  int r3 = find(db, "clink/clink.h");

  // close the database
  if (rc == 0)
    clink_db_close(&db);

  ASSERT_EQ(r1, 0); // failed to lookup include by full path
  ASSERT_EQ(r2, 0); // failed to lookup include by stem
  ASSERT_EQ(r3, 0); // failed to lookup include by suffix
}
