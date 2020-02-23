#pragma once

#include <string>

namespace clink {

struct Symbol {

  enum Category {
    DEFINITION,
    FUNCTION_CALL,
    REFERENCE,
    INCLUDE,
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
