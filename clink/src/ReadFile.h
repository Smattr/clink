#pragma once

#include <cstddef>
#include <clink/clink.h>
#include <filesystem>
#include "Task.h"
#include "WorkQueue.h"

// task for reading a file’s content
class ReadFile : public Task {

 private:
  std::filesystem::path path;

 public:
  ReadFile(const std::filesystem::path &path_);

  void run(clink::Database &db, WorkQueue &q) final;

  virtual ~ReadFile() = default;
};
