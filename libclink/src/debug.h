#pragma once

#include "../../common/compiler.h"
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

extern FILE *clink_debug INTERNAL;

/// emit a debug message
#define DEBUG(...)                                                             \
  do {                                                                         \
    if (UNLIKELY(clink_debug != NULL)) {                                       \
      const char *name_ = strrchr(__FILE__, '/');                              \
      flockfile(clink_debug);                                                  \
      fprintf(clink_debug, "[CLINK] libclink/src%s:%d: ", name_, __LINE__);    \
      fprintf(clink_debug, __VA_ARGS__);                                       \
      fprintf(clink_debug, "\n");                                              \
      funlockfile(clink_debug);                                                \
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
