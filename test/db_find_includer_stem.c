#include "test.h"
#include <assert.h>
#include <clink/clink.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

static const char PATH[] = "include/clink/clink.h";

static void find(clink_db_t *db, const char *path) {

  assert(db != NULL);
  assert(path != NULL);

  clink_iter_t *it = NULL;
  {
    int rc = clink_db_find_includer(db, path, &it);
    if (rc)
      fprintf(stderr, "clink_db_find_includer: %s\n", strerror(rc));
    ASSERT_EQ(rc, 0);
  }

  // confirm this iterator finds the expected record
  {
    const clink_symbol_t *sym = NULL;
    int rc = clink_iter_next_symbol(it, &sym);
    if (rc)
      fprintf(stderr, "clink_iter_next_symbol: %s\n", strerror(rc));
    ASSERT_EQ(rc, 0);

    ASSERT_EQ((int)sym->category, (int)CLINK_INCLUDE);
    ASSERT_STREQ(sym->name, PATH);
    ASSERT_EQ(sym->lineno, 42u);
    ASSERT_EQ(sym->colno, 10u);
    ASSERT_STREQ(sym->path, "/foo/bar");
    ASSERT_STREQ(sym->parent, "sym-parent");
  }

  // confirm that the iterator is now empty
  {
    const clink_symbol_t *sym = NULL;
    int rc = clink_iter_next_symbol(it, &sym);
    if (rc == 0) {
      fprintf(stderr, "iterator unexpectedly is non-empty\n");
    } else if (rc != ENOMSG) {
      fprintf(stderr, "clink_iter_next_symbol: %s\n", strerror(rc));
    }
    ASSERT_EQ(rc, ENOMSG);
  }

  clink_iter_free(&it);
}

TEST("test looking up an included file by its final component works (1)") {

  // construct a unique path
  char *target = test_tmpnam();

  // open it as a database
  clink_db_t *db = NULL;
  {
    int rc = clink_db_open(&db, target);
    if (rc)
      fprintf(stderr, "clink_db_open: %s\n", strerror(rc));
    ASSERT_EQ(rc, 0);
  }

  // add a record for the upcoming path
  char path[] = "/foo/bar";
  {
    int rc = clink_db_add_record(db, path, 0, 0, NULL);
    ASSERT_EQ(rc, 0);
  }

  // add an include that is a path with multiple components
  {
    clink_symbol_t symbol = {
        .category = CLINK_INCLUDE, .lineno = 42, .colno = 10};

    symbol.name = (char *)"include/clink/clink.h";
    symbol.path = path;
    symbol.parent = (char *)"sym-parent";

    int rc = clink_db_add_symbol(db, &symbol);
    if (rc)
      fprintf(stderr, "clink_db_add_symbol: %s\n", strerror(rc));
    ASSERT_EQ(rc, 0);
  }

  // lookup the include by full path
  find(db, "include/clink/clink.h");

  // close the database
  clink_db_close(&db);
}

TEST("test looking up an included file by its final component works (2)") {

  // construct a unique path
  char *target = test_tmpnam();

  // open it as a database
  clink_db_t *db = NULL;
  {
    int rc = clink_db_open(&db, target);
    if (rc)
      fprintf(stderr, "clink_db_open: %s\n", strerror(rc));
    ASSERT_EQ(rc, 0);
  }

  // add a record for the upcoming path
  char path[] = "/foo/bar";
  {
    int rc = clink_db_add_record(db, path, 0, 0, NULL);
    ASSERT_EQ(rc, 0);
  }

  // add an include that is a path with multiple components
  {
    clink_symbol_t symbol = {
        .category = CLINK_INCLUDE, .lineno = 42, .colno = 10};

    symbol.name = (char *)"include/clink/clink.h";
    symbol.path = path;
    symbol.parent = (char *)"sym-parent";

    int rc = clink_db_add_symbol(db, &symbol);
    if (rc)
      fprintf(stderr, "clink_db_add_symbol: %s\n", strerror(rc));
    ASSERT_EQ(rc, 0);
  }

  // lookup the include by stem
  find(db, "clink.h");

  // close the database
  clink_db_close(&db);
}

TEST("test looking up an included file by its final component works (3)") {

  // construct a unique path
  char *target = test_tmpnam();

  // open it as a database
  clink_db_t *db = NULL;
  {
    int rc = clink_db_open(&db, target);
    if (rc)
      fprintf(stderr, "clink_db_open: %s\n", strerror(rc));
    ASSERT_EQ(rc, 0);
  }

  // add a record for the upcoming path
  char path[] = "/foo/bar";
  {
    int rc = clink_db_add_record(db, path, 0, 0, NULL);
    ASSERT_EQ(rc, 0);
  }

  // add an include that is a path with multiple components
  {
    clink_symbol_t symbol = {
        .category = CLINK_INCLUDE, .lineno = 42, .colno = 10};

    symbol.name = (char *)"include/clink/clink.h";
    symbol.path = path;
    symbol.parent = (char *)"sym-parent";

    int rc = clink_db_add_symbol(db, &symbol);
    if (rc)
      fprintf(stderr, "clink_db_add_symbol: %s\n", strerror(rc));
    ASSERT_EQ(rc, 0);
  }

  // lookup the include by an intermediate suffix
  find(db, "clink/clink.h");

  // close the database
  clink_db_close(&db);
}
