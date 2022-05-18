#pragma once

#include "../../common/compiler.h"
#include "option.h"
#include <stdio.h>
#include <string.h>

/// emit a debug message
#define DEBUG(args...)                                                         \
  do {                                                                         \
    if (UNLIKELY(option.debug)) {                                              \
      const char *name_ = strrchr(__FILE__, '/');                              \
      flockfile(stderr);                                                       \
      fprintf(stderr, "[CLINK] clink/src%s:%d: ", name_, __LINE__);            \
      fprintf(stderr, args);                                                   \
      fprintf(stderr, "\n");                                                   \
      funlockfile(stderr);                                                     \
    }                                                                          \
  } while (0)
