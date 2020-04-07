#pragma once

#include <cstddef>
#include <clink/clink.h>
#include <filesystem>
#include "Task.h"
#include "WorkQueue.h"

// task for parsing an ASM file
class ParseAsm : public Task {

 private:
  std::filesystem::path path;

 public:
  ParseAsm(const std::filesystem::path &path_);

  void run(clink::Database &db, WorkQueue &q) final;

  virtual ~ParseAsm() = default;
};
