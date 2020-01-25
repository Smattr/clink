#pragma once

#define CLINK_UNUSED __attribute__((unused))

#ifdef __clang__
  #define CLINK_NONNULL _Nonnull
  #define CLINK_NULLABLE _Nullable
#else
  #define CLINK_NONNULL /* nothing */
  #define CLINK_NULLABLE /* nothing */
#endif
