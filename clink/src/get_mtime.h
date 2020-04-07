#pragma once

#include <cstddef>
#include <filesystem>

// get the last modified time of a file, or 0 if it does not exist
time_t get_mtime(const std::filesystem::path &p);
