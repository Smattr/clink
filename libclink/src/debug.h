#pragma once

#include "../../common/compiler.h"
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

extern FILE *debug INTERNAL;

/// emit a debug message
#define DEBUG(args...)                                                         \
  do {                                                                         \
    if (UNLIKELY(debug != NULL)) {                                             \
      const char *name_ = strrchr(__FILE__, '/');                              \
      flockfile(debug);                                                        \
      fprintf(debug, "[CLINK] libclink/src%s:%d: ", name_, __LINE__);          \
      fprintf(debug, args);                                                    \
      fprintf(debug, "\n");                                                    \
      funlockfile(debug);                                                      \
    }                                                                          \
  } while (0)

/// logging wrapper for error conditions
#define ERROR(cond)                                                            \
  ({                                                                           \
    bool cond_ = (cond);                                                       \
    if (UNLIKELY(cond_)) {                                                     \
      int errno_ = errno;                                                      \
      DEBUG("`%s` failed (current errno is %d)", #cond, errno_);               \
      errno = errno_;                                                          \
    }                                                                          \
    cond_;                                                                     \
  })
