#include <assert.h>
#include <clink/clink.h>
#include "colour.h"
#include <ctype.h>
#include <errno.h>
#include "line_ui.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/// transfer results from an iterator to stdout and free the iterator
static int drain_iter(clink_iter_t *it, bool use_parent) {

  assert(it != NULL);

  int rc = 0;

  // create a dynamically expanding buffer which we can store results in
  char *buffer = NULL;
  size_t buffer_size = 0;
  FILE *buf = open_memstream(&buffer, &buffer_size);
  if (buf == NULL) {
    rc = errno;
    goto done;
  }

  // total of how many symbols we have seen
  size_t count = 0;

  while (clink_iter_has_next(it)) {

    // retrieve the next symbol
    const clink_symbol_t *symbol = NULL;
    if ((rc = clink_iter_next_symbol(it, &symbol)))
      goto done;

    // strip leading white space from the context field
    const char *context = symbol->context;
    if (context == NULL)
      context = "\n";
    while (isspace(context[0]) && context[0] != '\n')
      ++context;

    // buffer the symbol’s details
    if (fprintf(buf, "%s %s %lu ", symbol->path,
        use_parent ? symbol->parent : symbol->name, symbol->lineno) < 0) {
      rc = errno;
      goto done;
    }

    // print context, stripping ANSI codes because Vim will not like them
    if ((rc = printf_bw(context, buf)))
      goto done;

    ++count;
  }

  // now that we know the total count, we can flush the results
  printf("cscope: %zu lines\n", count);

  (void)fclose(buf);
  buf = NULL;
  printf("%s", buffer);

done:
  if (buf)
    (void)fclose(buf);
  free(buffer);
  clink_iter_free(&it);

  return rc;
}

/// transfer results from a string iterator to stdout and free the iterator
static int drain_str_iter(clink_iter_t *it) {

  assert(it != NULL);

  int rc = 0;

  // create a dynamically expanding buffer which we can store results in
  char *buffer = NULL;
  size_t buffer_size = 0;
  FILE *buf = open_memstream(&buffer, &buffer_size);
  if (buf == NULL) {
    rc = errno;
    goto done;
  }

  // total of how many strings we have seen
  size_t count = 0;

  while (clink_iter_has_next(it)) {

    // retrieve the next string
    const char *s = NULL;
    if ((rc = clink_iter_next_str(it, &s)))
      goto done;

    /* XXX: what kind of nonsense output is this? I do not know what value
     * Cscope is attempting to add with the trailing garbage.
     */
    if (fprintf(buf, "%s <unknown> 1 <unknown>\n", s) < 0) {
      rc = errno;
      goto done;
    }

    ++count;
  }

  // now that we know the total count, we can flush the results
  printf("cscope: %zu lines\n", count);

  (void)fclose(buf);
  buf = NULL;
  printf("%s", buffer);

done:
  if (buf)
    (void)fclose(buf);
  free(buffer);
  clink_iter_free(&it);

  return rc;
}

int line_ui(clink_db_t *db) {

  assert(db != NULL);

  int rc = 0;
  char *line = NULL;
  size_t line_size = 0;

  for (;;) {

    // print prompt text
    printf(">> ");
    fflush(stdout);

    // read user input
    errno = 0;
    if (getline(&line, &line_size, stdin) < 0) {
      rc = errno;
      break;
    }

    // strip newline
    if (strlen(line) > 0 && line[strlen(line) - 1] == '\n')
      line[strlen(line) - 1] = '\0';

    // skip leading white space
    const char *command = line;
    while (isspace(command[0]))
      ++command;

    // ignore blank lines
    if (strcmp(command, "") == 0)
      continue;

    switch (line[0]) {

      case '0': { // find symbol
        clink_iter_t *it = NULL;
        if ((rc = clink_db_find_symbol(db, command + 1, &it)))
          break;
        rc = drain_iter(it, true);
        break;
      }

      case '1': { // find definition
        clink_iter_t *it = NULL;
        if ((rc = clink_db_find_definition(db, command + 1, &it)))
          break;
        rc = drain_iter(it, false);
        break;
      }

      case '2': { // find calls
        clink_iter_t *it = NULL;
        if ((rc = clink_db_find_call(db, command + 1, &it)))
          break;
        rc = drain_iter(it, false);
        break;
      }

      case '3': { // find callers
        clink_iter_t *it = NULL;
        if ((rc = clink_db_find_caller(db, command + 1, &it)))
          break;
        rc = drain_iter(it, true);
        break;
      }

      case '7': { // find file
        clink_iter_t *it = NULL;
        if ((rc = clink_db_find_file(db, command + 1, &it)))
          break;
        rc = drain_str_iter(it);
        break;
      }

      case '8': { // find includers
        clink_iter_t *it = NULL;
        if ((rc = clink_db_find_includer(db, command + 1, &it)))
          break;
        rc = drain_iter(it, true);
        break;
      }

      // Commands we do not support. Just pretend there were no results.
      case '4': // find text
      case '5': // change text
      case '6': // find pattern
      case '9': // find assignments
        printf("cscope: 0 lines\n");
        break;

      /* Bail out on any unrecognised command, under the assumption Vim would
       * never send us a malformed command.
       */
      default:
        rc = EINVAL;
        break;
    }

    if (rc)
      break;
  }

  free(line);

  return rc;
}
