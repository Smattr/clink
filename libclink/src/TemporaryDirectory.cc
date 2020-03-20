#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <new>
#include <stdexcept>
#include <string>
#include "TemporaryDirectory.h"
#include <unistd.h>

namespace clink {

TemporaryDirectory::TemporaryDirectory() {

  const char *TMPDIR = getenv("TMPDIR");
  if (TMPDIR == nullptr) {
    TMPDIR = "/tmp";
  }

  char *temp;
  if (asprintf(&temp, "%s/tmp.XXXXXX", TMPDIR) < 0)
    throw std::bad_alloc();

  if (mkdtemp(temp) == nullptr) {
    free(temp);
    throw std::invalid_argument(strerror(errno));
  }

  dir = temp;
  free(temp);
}

const std::string &TemporaryDirectory::get_path() const {
  return dir;
}

TemporaryDirectory::~TemporaryDirectory() {
  (void)rmdir(dir.c_str());
}

}
