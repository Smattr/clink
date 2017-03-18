#pragma once

#include <cstdio>

extern FILE *log_file;

#define LOG(args...) \
  do { \
    if (log_file != nullptr) { \
      fprintf(log_file, args); \
    } \
  } while (0)
