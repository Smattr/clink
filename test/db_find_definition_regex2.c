#include "test.h"
#include <clink/db.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

TEST("ensure clink_db_find_definition() with a regex restricts to start and "
     "end") {

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

  // add another new symbol that is not a definition
  if (rc == 0) {

    clink_symbol_t symbol = {
        .category = CLINK_INCLUDE, .lineno = 42, .colno = 10};

    symbol.name = (char *)"sym-name2";
    symbol.path = (char *)"/foo/bar";
    symbol.parent = (char *)"sym-parent";

    rc = clink_db_add_symbol(db, &symbol);
    if (rc)
      fprintf(stderr, "clink_db_add_symbol: %s\n", strerror(rc));
  }

  // add another symbol who has the first symbol as a suffix
  if (rc == 0) {

    clink_symbol_t symbol = {
        .category = CLINK_DEFINITION, .lineno = 42, .colno = 10};

    symbol.name = (char *)"another-sym-name";
    symbol.path = (char *)"/foo/bar";
    symbol.parent = (char *)"sym-parent";

    rc = clink_db_add_symbol(db, &symbol);
    if (rc)
      fprintf(stderr, "clink_db_add_symbol: %s\n", strerror(rc));
  }

  int r1 = 0;
  do {

    // lookup definition by regex
    clink_iter_t *it = NULL;
    if ((r1 = clink_db_find_definition(db, "sym-.*", &it))) {
      fprintf(stderr, "clink_db_find_definition: %s\n", strerror(r1));
      break;
    }

    // confirm this iterator finds something
    {
      const clink_symbol_t *sym = NULL;
      if ((r1 = clink_iter_next_symbol(it, &sym))) {
        fprintf(stderr, "clink_iter_next_symbol: %s\n", strerror(r1));
        break;
      }

      if (sym->category != CLINK_DEFINITION) {
        fprintf(stderr, "incorrect symbol category: %d != %d\n",
                (int)sym->category, (int)CLINK_DEFINITION);
        r1 = -1;
      }

      if (strcmp(sym->name, "sym-name") != 0) {
        fprintf(stderr, "incorrect symbol name: \"%s\" != \"sym-name\"\n",
                sym->name);
        r1 = -1;
      }

      if (sym->lineno != 42) {
        fprintf(stderr, "incorrect symbol line number: %lu != 42\n",
                sym->lineno);
        r1 = -1;
      }

      if (sym->colno != 10) {
        fprintf(stderr, "incorrect symbol column number: %lu != 10\n",
                sym->colno);
        r1 = -1;
      }

      if (strcmp(sym->path, "/foo/bar") != 0) {
        fprintf(stderr, "incorrect symbol path: \"%s\" != \"/foo/bar\"\n",
                sym->path);
        r1 = -1;
      }

      if (strcmp(sym->parent, "sym-parent") != 0) {
        fprintf(stderr, "incorrect symbol parent: \"%s\" != \"sym-parent\"\n",
                sym->parent);
        r1 = -1;
      }
    }

    // confirm that the iterator is now empty, and thus the regex had an
    // implicit leading `^`
    {
      const clink_symbol_t *sym = NULL;
      int r = clink_iter_next_symbol(it, &sym);
      if (r == 0) {
        fprintf(stderr, "iterator unexpectedly not empty\n");
        r1 = -1;
      } else if (r != ENOMSG) {
        fprintf(stderr, "clink_iter_next_symbol: %s\n", strerror(r));
        r1 = -1;
      }
    }

    clink_iter_free(&it);
  } while (0);

  // close the database
  if (rc == 0)
    clink_db_close(&db);

  // confirm that the database was opened correctly
  ASSERT_EQ(rc, 0);

  // confirm that lookup worked as expected
  ASSERT_EQ(r1, 0);
}
