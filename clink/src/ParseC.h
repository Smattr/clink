#pragma once

#include <clink/clink.h>
#include <filesystem>
#include "Task.h"
#include "WorkQueue.h"

// task for parsing a C/C++ file
class ParseC : public Task {

 private:
  std::filesystem::path path;

 public:
  ParseC(const std::filesystem::path &path_);

  void run(clink::Database &db, WorkQueue &q) final;

  virtual ~ParseC() = default;
};
