#pragma once

#include "../../common/compiler.h"
#include <stdbool.h>

/** run a command and return its exit status
 *
 * \param argv Argument list of program to run
 * \param mask_stdout Hide the process’ standard output and error?
 * \returns The process’ exit status or an errno
 */
INTERNAL int run(const char **argv, bool mask_stdout);
