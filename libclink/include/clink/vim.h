#pragma once

#include <string>

namespace clink {

/** open Vim at the given position in the given file
 *
 * \param filename File to open with Vim
 * \param lineno Line number to position cursor at within the file
 * \param colno Column number to position cursor at within the file
 * \returns Vim’s exit status
 */
int vim_open(const std::string &filename, unsigned long lineno,
  unsigned long colno);

}
