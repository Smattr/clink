#include "scanner.h"
#include "isid.h"
#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

bool eat_eol(scanner_t *s) {

  assert(s->base != NULL && "corrupted scanner state");
  assert(s->offset <= s->size && "corrupted scanner state");

  if (s->offset == s->size)
    return false;

  if (s->base[s->offset] == '\r') {
    if (s->offset + 1 < s->size && s->base[s->offset + 1] == '\n') {
      // Windows line ending
      ++s->offset;
    }
    ++s->lineno;
    s->colno = 1;
    ++s->offset;
    return true;
  }

  if (s->base[s->offset] == '\n') {
    ++s->lineno;
    s->colno = 1;
    ++s->offset;
    return true;
  }

  return false;
}

bool eat_id(scanner_t *s, const char *expected) {

  assert(s->base != NULL && "corrupted scanner state");
  assert(s->offset <= s->size && "corrupted scanner state");
  assert(expected != NULL);
  assert(strlen(expected) > 0);

#ifndef NDEBUG
  for (size_t i = 0; i < strlen(expected); ++i) {
    if (i == 0) {
      assert(isid0(expected[i]) &&
             "non-identifier character in expected identifier");
    } else {
      assert(isid(expected[i]) &&
             "non-identifier character in expected identifier");
    }
  }
#endif

  // speculatively advance the scanner over the expected text
  scanner_t spec = *s;
  if (!eat_if(&spec, expected))
    return false;

  // now we should be at the end of a word
  if (spec.offset < spec.size && isid(spec.base[spec.offset]))
    return false;

  *s = spec;
  return true;
}

bool eat_if(scanner_t *s, const char *expected) {

  assert(s->base != NULL && "corrupted scanner state");
  assert(s->offset <= s->size && "corrupted scanner state");
  assert(expected != NULL);
  assert(strlen(expected) > 0);

#ifndef NDEBUG
  for (size_t i = 0; i < strlen(expected); ++i) {
    if (expected[i] == '\r')
      assert(i + 1 != strlen(expected) && expected[i + 1] == '\n' &&
             "orphaned CR in expected text");
  }
#endif

  if (s->size - s->offset < strlen(expected))
    return false;

  if (strncmp(&s->base[s->offset], expected, strlen(expected)) != 0)
    return false;

  s->offset += strlen(expected);

  for (size_t i = 0; i < strlen(expected); ++i) {
    if (strncmp(&expected[i], "\r\n", strlen("\r\n")) == 0) {
      ++i;
      ++s->lineno;
      s->colno = 1;
    } else if (expected[i] == '\n') {
      ++s->lineno;
      s->colno = 1;
    } else {
      ++s->colno;
    }
  }

  return true;
}

void eat_one(scanner_t *s) {

  assert(s->base != NULL && "corrupted scanner state");
  assert(s->offset <= s->size && "corrupted scanner state");
  assert(s->offset < s->size && "advancing an exhausted scanner");

  if (eat_eol(s))
    return;

  bool is_escape = s->base[s->offset] == '\\';

  ++s->colno;
  ++s->offset;

  // handle escaped newlines such that the caller can remain oblivious
  if (is_escape)
    (void)eat_eol(s);
}

void eat_ws(scanner_t *s) {

  assert(s->base != NULL && "corrupted scanner state");
  assert(s->offset <= s->size && "corrupted scanner state");

  while (s->offset < s->size && isspace(s->base[s->offset]))
    eat_one(s);
}

static bool isspace_not_eol(char c) {
  return isspace(c) && c != '\n' && c != '\r';
}

void eat_ws_to_eol(scanner_t *s) {

  assert(s->base != NULL && "corrupted scanner state");
  assert(s->offset <= s->size && "corrupted scanner state");

  while (s->offset < s->size && isspace_not_eol(s->base[s->offset]))
    eat_one(s);
}
