#include <cstddef>
#include <clink/Error.h>
#include <optional>
#include "Re.h"
#include <regex.h>
#include <string>
#include <vector>

namespace clink {

Re::Re(const std::string &re) {
  int rc = regcomp(&regex, re.c_str(), REG_EXTENDED);
  if (rc != 0)
    throw Error("regular expression compilation failed", rc);
}

std::optional<std::vector<Re::Match>> Re::match(const std::string &s,
    unsigned expected) const {

  // allocate space for the number of matches we anticipate
  std::vector<regmatch_t> matches(expected);

  // try to match the input against our regex
  int rc = regexec(&regex, s.c_str(), expected, matches.data(), 0);

  // if the input did not match, we are done
  if (rc != 0)
    return {};

  // construct a representation of these matches
  std::vector<Match> output(expected);
  for (size_t i = 0; i < matches.size(); ++i)
    output[i] = Match{ static_cast<size_t>(matches[i].rm_so),
                       static_cast<size_t>(matches[i].rm_eo) };

  return output;
}

Re::~Re() {
  regfree(&regex);
}

}
