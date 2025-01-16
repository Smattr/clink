#pragma once

#include "../../common/compiler.h"
#include "option.h"
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/// emit a debug message
#define DEBUG(...)                                                             \
  do {                                                                         \
    if (UNLIKELY(option.debug)) {                                              \
      const char *name_ = strrchr(__FILE__, '/');                              \
      flockfile(stderr);                                                       \
      fprintf(stderr, "[CLINK] clink/src%s:%d: ", name_, __LINE__);            \
      fprintf(stderr, __VA_ARGS__);                                            \
      fprintf(stderr, "\n");                                                   \
      funlockfile(stderr);                                                     \
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
