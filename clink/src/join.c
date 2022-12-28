#include "../../common/compiler.h"
#include "path.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
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

  // construct the joined path
  size_t suffix_len = strlen(suffix);
  char *p = malloc(prefix_len + 1 + suffix_len + 1);
  if (UNLIKELY(p == NULL))
    return ENOMEM;
  memcpy(p, branch, prefix_len);
  p[prefix_len] = '/';
  memcpy(&p[prefix_len + 1], suffix, suffix_len + 1);

  *path = p;

  return 0;
}
