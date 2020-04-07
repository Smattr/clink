#include <cstddef>
#include <filesystem>
#include "get_mtime.h"
#include <sys/stat.h>

time_t get_mtime(const std::filesystem::path &p) {

  if (!std::filesystem::exists(p))
    return 0;

  struct stat s;
  if (stat(p.string().c_str(), &s) < 0)
    return 0;

  return s.st_mtime;
}
