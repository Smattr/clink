#pragma once

#include <cstddef>
#include <string>

namespace clink {

// RAII wrapper around `mktemp -d`
class TemporaryDirectory {

 public:
  TemporaryDirectory();

  const std::string &get_path() const;

  ~TemporaryDirectory();

 private:
  std::string dir;

};

}
