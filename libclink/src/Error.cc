#include <cstddef>
#include <clink/Error.h>
#include <string>

namespace clink {

Error::Error(const std::string &message, int code_):
  std::runtime_error(message + (code_ == 0 ? "" : (" (code "
    + std::to_string(code_) + ")"))), code(code_) { }

}
