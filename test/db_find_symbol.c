#include "test.h"
#include <clink/db.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

TEST("clink_db_find_symbol()") {

  // construct a unique path
  char *target = test_tmpnam();

  // open it as a database
  clink_db_t *db = NULL;
  int rc = clink_db_open(&db, target);
  if (rc)
    fprintf(stderr, "clink_db_open: %s\n", strerror(rc));

  // add a new symbol
  if (rc == 0) {

    clink_symbol_t symbol = {
        .category = CLINK_DEFINITION, .lineno = 42, .colno = 10};

    symbol.name = (char *)"sym-name";
    symbol.path = (char *)"/foo/bar";
    symbol.parent = (char *)"sym-parent";

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
    const clink_symbol_t *sym = NULL;
    int r = clink_iter_next_symbol(it, &sym);
    if (r == 0) {
      fprintf(stderr, "iterator unexpectedly is non-empty\n");
      r1 = -1;
    } else if (r != ENOMSG) {
      fprintf(stderr, "clink_iter_next_symbol: %s\n", strerror(r));
      r1 = -1;
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
    {
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
    {
      const clink_symbol_t *sym = NULL;
      int r = clink_iter_next_symbol(it, &sym);
      if (r == 0) {
        fprintf(stderr, "iterator unexpectedly is non-empty\n");
        r2 = -1;
      } else if (r != ENOMSG) {
        fprintf(stderr, "clink_iter_next_symbol: %s\n", strerror(r));
        r2 = -1;
      }
    }

    clink_iter_free(&it);
  } while (0);

  // close the database
  if (rc == 0)
    clink_db_close(&db);

  // confirm that the database was opened correctly
  ASSERT_EQ(rc, 0);

  // confirm that lookup of a missing symbol worked as expected
  ASSERT_EQ(r1, 0);

  // confirm that lookup of a symbol that does exist works as expected
  ASSERT_EQ(r2, 0);
}
