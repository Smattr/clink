#include <errno.h>
#include "re.h"
#include <regex.h>

int re_err_to_errno(int err) {
  switch (err) {

    case 0: return 0;

    // not finding a match is not an error
    case REG_NOMATCH: return 0;

#ifdef __linux__
    case REG_ESIZE:
#endif
    case REG_ESPACE: return ENOMEM;

    // all other regex errors related to the regex itself being invalid, and all
    // our regexes are static constant strings, so consider any such error to be
    // an internal Clink fault
    default: return ENOTRECOVERABLE;
  }
}
