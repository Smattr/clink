#include <assert.h>
#include <clink/error.h>
#include "error.h"
#include <regex.h>
#include <sqlite3.h>
#include <stdint.h>
#include <string.h>

int sqlite_error(int err) {
  assert(err >= 0 && err <= SQLITE_DONE && "SQLite error out of range");
  return -err;
}

int regex_error(int err) {
  assert(err < 128 && "unexpected POSIX regex error code");
  return err << 8;
}

// errors defined by POSIX regex
static const char *REGERRORS[] = {
  [REG_NOMATCH]  = "regexec() failed to match",
  [REG_BADPAT]   = "invalid regular expression",
  [REG_ECOLLATE] = "invalid collating element referenced",
  [REG_ECTYPE]   = "invalid character class type referenced",
  [REG_EESCAPE]  = "trailing <backslash> character in pattern",
  [REG_ESUBREG]  = "number in \\digit invalid or in error",
  [REG_EBRACK]   = "\"[]\" imbalance",
  [REG_EPAREN]   = "\"\\(\\)\" of \"()\" imbalance",
  [REG_EBRACE]   = "\"\\{\\}\" imbalance",
  [REG_BADBR]    = "content of \"\\{\\}\" invalid: not a number, number too "
                   "large, more than two numbers, first larger than second",
  [REG_ERANGE]   = "invalid endpoint in range expression",
  [REG_ESPACE]   = "out of memory",
  [REG_BADRPT]   = "'?', '*', or '+' not preceded by valid regular expression",
};

const char *clink_strerror(int err) {

  // if this is a negative number, it is an encoded SQLite or regex error
  if (err < 0) {
    int e = -err;

    // if it is less than the maximum SQLite error code, treat it as a SQLite
    // error
    if (e <= SQLITE_DONE)
      return sqlite3_errstr(e);

    // otherwise assume it is a POSIX error code, shifted out of the first byte
    int regex_code = e >> 8;

    if (regex_code < (int)(sizeof(REGERRORS) / sizeof(REGERRORS[0])))
      return REGERRORS[regex_code];

    return "unknown";
  }

  // if it is non-negative, assume it is an errno
  return strerror(err);
}
