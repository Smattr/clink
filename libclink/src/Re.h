#pragma once

#include <cstddef>
#include <optional>
#include <regex.h>
#include <string>

namespace clink {

// RAII wrapper around POSIX regex
class Re {

 public:
  explicit Re(const std::string &re);

  // result of a regex match
  struct Match {
    size_t start_offset;
    size_t end_offset;
  };

  std::optional<Match> match(const std::string &s) const;

  ~Re();

 private:
  regex_t regex;
};

}
