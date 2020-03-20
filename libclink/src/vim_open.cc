#include <clink/vim.h>
#include <errno.h>
#include "run.h"
#include <sstream>
#include <string>

namespace clink {

int vim_open(const std::string &filename, unsigned long lineno,
    unsigned long colno) {

  // check line number is valid
  if (lineno == 0)
    return ERANGE;

  // check column number is valid
  if (colno == 0)
    return ERANGE;

  // construct a directive telling Vim to jump to the given position
  std::ostringstream cursor;
  cursor << "+call cursor(" << lineno << "," << colno << ")";

  // construct a argument vector to invoke Vim
  char const *argv[]
    = { "vim", cursor.str().c_str(), filename.c_str(), nullptr };

  // run it
  return run(argv, false);
}

}
