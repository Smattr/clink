#pragma once

#include <string>

namespace clink {

struct Symbol {

  enum Category {
    DEFINITION = 0,
    FUNCTION_CALL = 1,
    REFERENCE = 2,
    INCLUDE = 3,
  };

  Category category;
  std::string name;
  std::string path;
  unsigned long lineno;
  unsigned long colno;
  std::string parent;
};

struct Result {
  Symbol symbol;
  std::string context;
};

}
