#pragma once

#include <cstdio>
#include <mutex>

extern FILE *log_file;
extern std::mutex log_lock;

#define LOG(args...) \
  do { \
    if (log_file != nullptr) { \
      std::lock_guard<std::mutex> _guard(log_lock); \
      fprintf(log_file, "%s:%d: ", __func__, __LINE__); \
      fprintf(log_file, args); \
      fprintf(log_file, "\n"); \
      fflush(log_file); \
    } \
  } while (0)
