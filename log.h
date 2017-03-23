#pragma once

#include <cstdio>

extern FILE *log_file;

#define LOG(args...) \
  do { \
    if (log_file != nullptr) { \
      fprintf(log_file, "%s:%d: ", __func__, __LINE__); \
      fprintf(log_file, args); \
      fprintf(log_file, "\n"); \
    } \
  } while (0)
