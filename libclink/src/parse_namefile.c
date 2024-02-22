#include "../../common/compiler.h"
#include "debug.h"
#include <clink/cscope.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/// default `accept` callback if none is provided
static int noop_accept(const char *name UNUSED, void *context UNUSED) {
  return 0;
}

/// default `error` callback if none is provided
static int noop_error(unsigned long lineno UNUSED, unsigned long colno UNUSED,
                      const char *message UNUSED, void *context UNUSED) {
  return 0;
}

static bool is_separator(int c) {
  return c == ' ' || c == '\t' || c == '\n' || c == EOF;
}

int clink_parse_namefile(FILE *in,
                         int (*accept)(const char *name, void *context),
                         int (*error)(unsigned long lineno, unsigned long colno,
                                      const char *message, void *context),
                         void *context) {

  if (ERROR(in == NULL))
    return EINVAL;

  if (accept == NULL)
    accept = noop_accept;

  if (error == NULL)
    error = noop_error;

  char *buffer = NULL;
  size_t buffer_size = 0;
  FILE *stage = NULL;
  int rc = 0;

  stage = open_memstream(&buffer, &buffer_size);
  if (ERROR(stage == NULL)) {
    rc = errno;
    goto done;
  }

  // state machine for processing entries
  enum { IDLE, IN_UNQUOTED, IN_QUOTED } state = IDLE;

  for (unsigned long lineno = 1, colno = 1;;) {

    int c = getc(in);
    ++colno;

    if (c == EOF && state == IN_QUOTED) {
      rc = error(lineno, colno,
                 "unexpected end-of-file in the middle of quoted string",
                 context);
      if (rc == 0)
        rc = EIO;
      goto done;
    }

    // does this end the current name?
    if ((state == IN_UNQUOTED && is_separator(c)) ||
        (state == IN_QUOTED && c == '\"')) {
      if (ERROR(fflush(stage) < 0)) {
        rc = errno;
        goto done;
      }
      if (ERROR((rc = accept(buffer, context))))
        goto done;
      memset(buffer, 0, strlen(buffer));
      rewind(stage);
      if (c == EOF)
        break;
      state = IDLE;
      goto skip;
    }

    if (c == EOF)
      break;

    switch (state) {
    case IDLE:
      if (is_separator(c))
        goto skip;
      if (c == '"') {
        state = IN_QUOTED;
        goto skip;
      }
      state = IN_UNQUOTED;
      break;

    case IN_UNQUOTED:
      // nothing
      break;

    case IN_QUOTED:
      if (c == '\\') {
        c = getc(in);
        if (c != '\"' && c != '\\') {
          rc = error(lineno, colno, "malformed escaped sequence", context);
          if (rc == 0)
            rc = EIO;
          goto done;
        }
        ++colno;
      }
      break;
    }

    // accrue this character
    if (ERROR(fputc(c, stage) < 0)) {
      rc = ENOMEM;
      goto done;
    }

  skip:
    if (c == '\n') {
      ++lineno;
      colno = 0;
    }
  }

done:
  if (stage != NULL)
    (void)fclose(stage);
  free(buffer);

  return rc;
}
