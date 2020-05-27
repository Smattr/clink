#include <errno.h>
#include "path.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>

int join(const char *branch, const char *stem, char **path) {

  if (branch == NULL)
    return EINVAL;

  if (strcmp(branch, "") == 0)
    return EINVAL;

  if (stem == NULL)
    return EINVAL;

  if (strcmp(stem, "") == 0)
    return EINVAL;

  if (path == NULL)
    return EINVAL;

  // determine the prefix, stripping any trailing /s
  size_t prefix_len = strlen(branch);
  while (prefix_len > 0 && branch[prefix_len - 1] == '/')
    --prefix_len;

  // determining the suffix, stripping any leading /s
  const char *suffix = stem;
  while (*suffix == '/')
    ++suffix;

  if (asprintf(path, "%.*s/%s", (int)prefix_len, branch, suffix) < 0)
    return errno;

  return 0;
}
