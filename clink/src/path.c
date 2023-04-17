#include "path.h"
#include "option.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

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
  return strcasecmp(path + strlen(path) - strlen(ext), ext) == 0;
}

bool is_asm(const char *path) { return has_ext(path, "s"); }

bool is_c(const char *path) { return has_ext(path, "c") || has_ext(path, "h"); }

bool is_cxx(const char *path) {
  return has_ext(path, "c++") || has_ext(path, "cpp") || has_ext(path, "cxx") ||
         has_ext(path, "cc") || has_ext(path, "h") || has_ext(path, "hh") ||
         has_ext(path, "hpp");
}

bool is_def(const char *path) { return has_ext(path, "def"); }

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

bool is_python(const char *path) { return has_ext(path, "py"); }

bool is_source(const char *path) {

  if (path == NULL)
    return false;

  if (is_asm(path) && option.parse_asm != OFF)
    return true;

  if (is_def(path) && option.parse_def != OFF)
    return true;

  if (is_c(path) && option.parse_c != OFF)
    return true;

  if (is_cxx(path) && option.parse_cxx != OFF)
    return true;

  if (is_python(path) && option.parse_python != OFF)
    return true;

  if (is_tablegen(path) && option.parse_tablegen != OFF)
    return true;

  return false;
}

bool is_tablegen(const char *path) { return has_ext(path, "td"); }
