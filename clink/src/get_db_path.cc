#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <filesystem>
#include "get_db_path.h"
#include <iostream>
#include "Options.h"
#include <unistd.h>

std::filesystem::path get_db_path() {

  // if the user gave us a database path, just use that
  if (options.database_path.has_value())
    return *options.database_path;

  // get the current working directory
  char *cwd = getcwd(nullptr, 0);
  if (cwd == nullptr) {
    std::cerr << "failed to read current directory: " << strerror(errno) << "\n";
    exit(EXIT_FAILURE);
  }

  // walk up the directory tree looking for an existing .clink.db file
  std::filesystem::path branch = cwd;
  for (;;) {

    // form a path to .clink.db at our current level
    std::filesystem::path candidate = branch / ".clink.db";

    // does this file exist?
    if (std::filesystem::exists(candidate))
      return candidate;

    // if we are at the root directory, we have tried all possibilities
    if (branch.root_path() == branch)
      break;

    // move one directory up
    branch = branch.parent_path();
  }

  // fall back on just $(pwd)/.clink.db
  return std::filesystem::path(cwd) / ".clink.db";
}
