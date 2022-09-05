#include "re.h"
#include <errno.h>
#include <regex.h>
#include <stddef.h>

int re_check(const char *pattern) {

  if (pattern == NULL)
    return EINVAL;

  regex_t regex;
  int rc = regcomp(&regex, pattern, REG_EXTENDED | REG_NOSUB);

  switch (rc) {

  case 0:
    regfree(&regex);
    return 0;

#ifdef REG_ESIZE
  case REG_ESIZE:
#endif
  case REG_ESPACE:
    return ENOMEM;

  default:
    return EINVAL;
  }
}
