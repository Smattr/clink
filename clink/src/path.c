#include <ctype.h>
#include <errno.h>
#include "path.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

int abspath(const char *path, char **result) {

  if (path == NULL)
    return EINVAL;

  if (result == NULL)
    return EINVAL;

  // if the path is already absolute, we just need to duplicate it
  if (path[0] == '/') {
    *result = strdup(path);
    if (*result == NULL)
      return ENOMEM;
    return 0;
  }

  // otherwise, we need to prepend the current working directory

  char *wd = NULL;
  int rc = cwd(&wd);
  if (rc)
    return rc;

  int r = asprintf(result, "%s/%s", wd, path);
  if (r < 0)
    rc = errno;

  free(wd);
  return rc;
}

int cwd(char **wd) {

  if (wd == NULL)
    return EINVAL;

  char *w = getcwd(NULL, 0);
  if (w == NULL)
    return errno;

  *wd = w;
  return 0;
}

int disppath(const char *path, char **display) {

  if (path == NULL)
    return EINVAL;

  if (display == NULL)
    return EINVAL;

  // get the current working directory
  char *wd = NULL;
  int rc = cwd(&wd);
  if (rc)
    return rc;

  char *d = NULL;

  // if the given path was exactly the working directory, describe it as “.”
  if (strcmp(wd, path) == 0) {
    d = strdup(".");
    if (d == NULL)
      rc = ENOMEM;
    goto done;
  }

  // if wd is a prefix of the path, take the suffix as the display
  if (strncmp(wd, path, strlen(wd)) == 0 && path[strlen(wd)] == '/') {
    d = strdup(&path[strlen(wd) + 1]);
    if (d == NULL)
      rc = ENOMEM;
    goto done;
  }

  // otherwise just use the path itself
  d = strdup(path);
  if (d == NULL)
    rc = ENOMEM;

done:
  free(wd);

  if (rc == 0)
    *display = d;

  return rc;
}

/// does this path have the given file extension?
static bool has_ext(const char *path, const char *ext) {

  if (path == NULL || ext == NULL)
    return false;

  if (strlen(path) < strlen(ext) + strlen("."))
    return false;

  // does the extension begin where we expect?
  if (path[strlen(path) - strlen(ext) - strlen(".")] != '.')
    return false;

  // case-insensitive comparison of the extension
  for (size_t i = 0; i < strlen(ext); ++i) {
    if (tolower(path[strlen(path) - strlen(ext) + i]) != tolower(ext[i]))
      return false;
  }

  return true;
}

bool is_asm(const char *path) {
  return has_ext(path, "s");
}

bool is_c(const char *path) {
  return has_ext(path, "c")
      || has_ext(path, "c++")
      || has_ext(path, "cpp")
      || has_ext(path, "cxx")
      || has_ext(path, "cc")
      || has_ext(path, "h")
      || has_ext(path, "hpp");
}

bool is_dir(const char *path) {

  if (path == NULL)
    return false;

  struct stat buf;
  if (stat(path, &buf) < 0)
    return false;

  return S_ISDIR(buf.st_mode);
}

bool is_file(const char *path) {

  if (path == NULL)
    return false;

  struct stat buf;
  if (stat(path, &buf) < 0)
    return false;

  return S_ISREG(buf.st_mode);
}
