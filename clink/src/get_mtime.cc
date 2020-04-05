#include <filesystem>
#include "get_mtime.h"

time_t get_mtime(const std::filesystem::path &p) {

  if (!std::filesystem::exists(p))
    return 0;

  using std_time = std::filesystem::file_time_type;

  std_time t = std::filesystem::last_write_time(p);
  return std_time::clock::to_time_t(t);
}
