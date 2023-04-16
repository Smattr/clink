#include "test.h"
#include <clink/db.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

TEST("clink_db_find_call() with a regex") {

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

  // add a new symbol
  {
    clink_symbol_t symbol = {
        .category = CLINK_FUNCTION_CALL, .lineno = 42, .colno = 10};

    symbol.name = (char *)"sym-name";
    symbol.path = path;
    symbol.parent = (char *)"sym-parent";

    int rc = clink_db_add_symbol(db, &symbol);
    if (rc)
      fprintf(stderr, "clink_db_add_symbol: %s\n", strerror(rc));
    ASSERT_EQ(rc, 0);
  }

  // add another new symbol that is not a call
  {
    clink_symbol_t symbol = {
        .category = CLINK_DEFINITION, .lineno = 42, .colno = 10};

    symbol.name = (char *)"sym-name2";
    symbol.path = path;
    symbol.parent = (char *)"sym-parent";

    int rc = clink_db_add_symbol(db, &symbol);
    if (rc)
      fprintf(stderr, "clink_db_add_symbol: %s\n", strerror(rc));
    ASSERT_EQ(rc, 0);
  }

  {
    // lookup a call by regex
    clink_iter_t *it = NULL;
    {
      int rc = clink_db_find_call(db, "sym-p.*", &it);
      if (rc)
        fprintf(stderr, "clink_db_find_call: %s\n", strerror(rc));
      ASSERT_EQ(rc, 0);
    }

    // confirm this iterator finds something
    {
      const clink_symbol_t *sym = NULL;
      int rc = clink_iter_next_symbol(it, &sym);
      if (rc)
        fprintf(stderr, "clink_iter_next_symbol: %s\n", strerror(rc));
      ASSERT_EQ(rc, 0);

      ASSERT_EQ((int)sym->category, (int)CLINK_FUNCTION_CALL);
      ASSERT_STREQ(sym->name, "sym-name");
      ASSERT_EQ(sym->lineno, 42u);
      ASSERT_EQ(sym->colno, 10u);
      ASSERT_STREQ(sym->path, path);
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

  // close the database
  clink_db_close(&db);
}
