#pragma once

#include <cstddef>
#include <clink/Symbol.h>
#include <functional>
#include <string>
#include <vector>

namespace clink {

// parse a C/C++ file
//
// Returns 0 on success. If your callback ever returns non-zero, parsing will be
// terminated and this value will be returned.
int parse_c(const std::string &filename,
  const std::vector<std::string> &clang_args,
  std::function<int(const Symbol&)> const &callback);

}
