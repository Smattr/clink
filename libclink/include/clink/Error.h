#pragma once

#include <cstddef>
#include <stdexcept>
#include <string>

namespace clink {

class Error : public std::runtime_error {

 public:
  int code;

  explicit Error(const std::string &message, int code = 0);
};

}
