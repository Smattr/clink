#pragma once

#include <stdbool.h>

// run a command and return its exit status
//
// The mask_stdout argument controls whether the process’ standard input,
// output, and error streams are hidden.
__attribute__((visibility("internal")))
int run(const char **argv, bool mask_stdout);
