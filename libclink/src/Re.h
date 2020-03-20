#pragma once

#include <cstddef>
#include <array>
#include <climits>
#include <clink/Error.h>
#include <optional>
#include <regex.h>
#include <string>

namespace clink {

// result of a regex match
struct Match {
  size_t start_offset;
  size_t end_offset;
};

static inline size_t off_to_size(off_t offset) {
  if (offset < 0)
    return SIZE_MAX;
  return size_t(offset);
}

// RAII wrapper around POSIX regex
template<size_t MATCHES, size_t CORE = 1>
class Re {

 public:
  explicit Re(const std::string &re) {
    int rc = regcomp(&regex, re.c_str(), REG_EXTENDED);
    if (rc != 0)
      throw Error("regular expression compilation failed", rc);
  }

  std::optional<std::array<Match, MATCHES>> matches(const std::string &s) const {

    // allocate space for the number of matches we anticipate
    regmatch_t ms[MATCHES];
    size_t ms_len = sizeof(ms) / sizeof(ms[0]);

    // try to match the input against our regex
    int rc = regexec(&regex, s.c_str(), ms_len, ms, 0);

    // if the input did not match, we are done
    if (rc != 0)
      return {};

    // otherwise, construct a representation of all the matches
    std::array<Match, MATCHES> m;
    for (size_t i = 0; i < ms_len; i++)
      m[i] = Match{ off_to_size(ms[i].rm_so), off_to_size(ms[i].rm_eo) };

    return m;
  }

  std::optional<Match> match(const std::string &s) const {

    std::optional<std::array<Match, MATCHES>> ms = matches(s);

    if (ms.has_value())
      return (*ms)[CORE];

    return {};
  }

  ~Re() {
    regfree(&regex);
  }

 private:
  regex_t regex;
};

}
