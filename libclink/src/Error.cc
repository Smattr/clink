#include <cstddef>
#include <clink/Error.h>
#include <string>

namespace clink {

Error::Error(const std::string &message, int code):
  std::runtime_error(message), code(code) { }

}
