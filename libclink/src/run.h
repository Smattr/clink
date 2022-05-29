#pragma once

#include "../../common/compiler.h"

/** run a command and return its exit status
 *
 * \param argv Argument list of program to run
 * \returns The process’ exit status or an errno
 */
INTERNAL int run(const char **argv);
