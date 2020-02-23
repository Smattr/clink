#include <cstddef>
#include <clink/Error.h>
#include <optional>
#include "Re.h"
#include <regex.h>
#include <string>

namespace clink {

Re::Re(const std::string &re) {

  int rc = regcomp(&regex, re.c_str(), REG_EXTENDED);
  if (rc != 0)
    throw Error("regular expression compilation failed", rc);
}

std::optional<Re::Match> Re::match(const std::string &s) const {

  // allocate space for the number of matches we anticipate
  regmatch_t matches[2];
  size_t matches_len = sizeof(matches) / sizeof(matches[0]);

  // try to match the input against our regex
  int rc = regexec(&regex, s.c_str(), matches_len, matches, 0);

  // if the input did not match, we are done
  if (rc != 0)
    return {};

  // construct a representation of the match
  return Match{ static_cast<size_t>(matches[1].rm_so),
                static_cast<size_t>(matches[1].rm_eo) };
}

Re::~Re() {
  regfree(&regex);
}

}
