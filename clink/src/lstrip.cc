#include <cstddef>
#include <ctype.h>
#include "lstrip.h"
#include <string>

std::string lstrip(const std::string &s) {

  std::string t;

  bool dropping = true;
  for (const char &c : s) {
    if (dropping) {
      if (!isspace(c)) {
        t += c;
        dropping = false;
      }
    } else {
      t += c;
    }
  }

  return t;
}
