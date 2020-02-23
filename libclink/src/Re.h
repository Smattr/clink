#pragma once

#include <cstddef>
#include <clink/Error.h>
#include <optional>
#include <regex.h>
#include <string>

namespace clink {

// RAII wrapper around POSIX regex
template<size_t MATCHES, size_t CORE>
class Re {

 public:
  explicit Re(const std::string &re) {
    int rc = regcomp(&regex, re.c_str(), REG_EXTENDED);
    if (rc != 0)
      throw Error("regular expression compilation failed", rc);
  }

  // result of a regex match
  struct Match {
    size_t start_offset;
    size_t end_offset;
  };

  std::optional<Match> match(const std::string &s) const {

    // allocate space for the number of matches we anticipate
    regmatch_t matches[MATCHES];
    size_t matches_len = sizeof(matches) / sizeof(matches[0]);

    // try to match the input against our regex
    int rc = regexec(&regex, s.c_str(), matches_len, matches, 0);

    // if the input did not match, we are done
    if (rc != 0)
      return {};

    // construct a representation of the match
    return Match{ static_cast<size_t>(matches[CORE].rm_so),
                  static_cast<size_t>(matches[CORE].rm_eo) };
  }

  ~Re() {
    regfree(&regex);
  }

 private:
  regex_t regex;
};

}
