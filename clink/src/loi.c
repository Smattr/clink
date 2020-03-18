#include <assert.h>
#include <clink/clink.h>
#include <ctype.h>
#include <errno.h>
#include "loi.h"
#include "lstrip.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void free_results(struct clink_result *results, size_t results_size) {
  for (size_t i = 0; i < results_size; i++)
    clink_result_clear(&results[i]);
  free(results);
}

typedef struct {
  char **data;
  size_t size;
  size_t capacity;
} str_list;

static int accrue_string(const char *path, void *state) {

  str_list *sl = state;

  // do we need to expand the strings array?
  if (sl->capacity == sl->size) {
    size_t capacity = sl->capacity == 0 ? 1 : sl->capacity * 2;
    char **s = realloc(sl->data, capacity * sizeof(s[0]));
    if (s == NULL)
      return ENOMEM;
    sl->data = s;
    sl->capacity = capacity;
  }

  char *s = strdup(path);
  if (s == NULL)
    return ENOMEM;

  sl->data[sl->size] = s;
  sl->size++;

  return 0;
}

static void free_strings(str_list *sl) {
  for (size_t i = 0; i < sl->size; i++)
    free(sl->data[i]);
  free(sl->data);
  sl->data = NULL;
  sl->size = sl->capacity = 0;
}

static void print_leader(size_t size) {
  printf("cscope: %zu lines\n", size);
}

static int find_symbols(struct clink_db *db, const char *name,
    struct clink_result **rs, size_t *rs_size) {
  return clink_db_results(db, name, clink_db_find_symbol, rs, rs_size);
}

static int find_definitions(struct clink_db *db, const char *name,
    struct clink_result **rs, size_t *rs_size) {
  return clink_db_results(db, name, clink_db_find_definition, rs, rs_size);
}

static int find_calls(struct clink_db *db, const char *name,
    struct clink_result **rs, size_t *rs_size) {
  return clink_db_results(db, name, clink_db_find_call, rs, rs_size);
}

static int find_callers(struct clink_db *db, const char *name,
    struct clink_result **rs, size_t *rs_size) {
  return clink_db_results(db, name, clink_db_find_caller, rs, rs_size);
}

static int find_includers(struct clink_db *db, const char *name,
    struct clink_result **rs, size_t *rs_size) {
  return clink_db_results(db, name, clink_db_find_includer, rs, rs_size);
}

static int find_file(struct clink_db *db, const char *name, str_list *sl) {
  int rc = clink_db_find_file(db, name, accrue_string, sl);
  if (rc != 0)
    free_strings(sl);
  return rc;
}

int loi_main(struct clink_db *db) {

  int rc = EXIT_SUCCESS;

  // last line we read
  char *line = NULL;
  size_t line_size = 0;

  for (;;) {

    // output prompt
    printf(">> ");
    fflush(stdout);

    // wait for input from the user (or Vim)
    if (getline(&line, &line_size, stdin) < 0) {
      rc = errno;
      break;
    }

    // skip leading white space
    const char *command = line;
    while (isspace(*command))
      command++;

    // ignore blank lines
    if (strcmp(command, "") == 0)
      continue;

    switch (*command) {

      case '0': { // find symbol
        struct clink_result *rs = NULL;
        size_t rs_size = 0;
        if ((rc = find_symbols(db, command + 1, &rs, &rs_size)))
          goto done;
        print_leader(rs_size);
        for (size_t i = 0; i < rs_size; i++)
          printf("%s %s %lu %s", rs[i].symbol.path, rs[i].symbol.parent,
            rs[i].symbol.lineno, lstrip(rs[i].context));
        free_results(rs, rs_size);
        break;
      }

      case '1': { // find definition
        struct clink_result *rs = NULL;
        size_t rs_size = 0;
        if ((rc = find_definitions(db, command + 1, &rs, &rs_size)))
          goto done;
        print_leader(rs_size);
        for (size_t i = 0; i < rs_size; i++)
          printf("%s %s %lu %s", rs[i].symbol.path, command + 1,
            rs[i].symbol.lineno, lstrip(rs[i].context));
        free_results(rs, rs_size);
        break;
      }

      case '2': { // find calls
        struct clink_result *rs = NULL;
        size_t rs_size = 0;
        if ((rc = find_calls(db, command + 1, &rs, &rs_size)))
          goto done;
        print_leader(rs_size);
        for (size_t i = 0; i < rs_size; i++)
          printf("%s %.*s %lu %s", rs[i].symbol.path,
            (int)rs[i].symbol.name_len, rs[i].symbol.name,
            rs[i].symbol.lineno, lstrip(rs[i].context));
        free_results(rs, rs_size);
        break;
      }

      case '3': { // find callers
        struct clink_result *rs = NULL;
        size_t rs_size = 0;
        if ((rc = find_callers(db, command + 1, &rs, &rs_size)))
          goto done;
        print_leader(rs_size);
        for (size_t i = 0; i < rs_size; i++)
          printf("%s %s %lu %s", rs[i].symbol.path, rs[i].symbol.parent,
            rs[i].symbol.lineno, lstrip(rs[i].context));
        free_results(rs, rs_size);
        break;
      }

      case '7': { // find file
        str_list sl = { 0 };
        if ((rc = find_file(db, command + 1, &sl)))
          goto done;
        print_leader(sl.size);
        // XXX: what kind of nonsense output is this? I do not know what value
        // Cscope is attempting to add with the trailing garbage.
        for (size_t i = 0; i < sl.size; i++)
          printf("%s <unknown> 1 <unknown>\n", sl.data[i]);
        free_strings(&sl);
        break;
      }

      case '8': { // find includers
        struct clink_result *rs = NULL;
        size_t rs_size = 0;
        if ((rc = find_includers(db, command + 1, &rs, &rs_size)))
          goto done;
        print_leader(rs_size);
        for (size_t i = 0; i < rs_size; i++)
          printf("%s %s %lu %s", rs[i].symbol.path, rs[i].symbol.parent,
            rs[i].symbol.lineno, lstrip(rs[i].context));
        free_results(rs, rs_size);
        break;
      }

      // Commands we do not support. Just pretend there were no results.
      case '4': // find text
      case '5': // change text
      case '6': // find pattern
      case '9': // find assignments
        print_leader(0);
        break;

      // bail out on any unrecognised command, under the assumption Vim would
      // never send us a malformed command
      default:
        rc = EINVAL;
        goto done;

    }
  }

done:
  free(line);

  if (rc != 0) {
    fprintf(stderr, "error: %s\n", clink_strerror(rc));
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
