#pragma once

#include <cstddef>
#include <filesystem>
#include <mutex>
#include <queue>
#include <stack>
#include <string>
#include <sys/types.h>
#include "Task.h"
#include <unordered_set>
#include <utility>

class WorkQueue {

 public:
  WorkQueue(const std::string &directory, time_t era_start_);
  std::unique_ptr<Task> pop();
  void push(const std::filesystem::path &path);

 private:
  time_t era_start;

  // pending directories to scan
  std::stack<std::pair<std::filesystem::directory_iterator,
                       std::filesystem::directory_iterator>> dirs;

  // pending files whose contents need to be read
  std::queue<std::filesystem::path> files_to_read;

  // files we have already encountered and enqueued
  std::unordered_set<std::string> files_seen;

  // add a directory to our pending list
  void push_dir(const std::filesystem::path &dir);

  std::mutex stack_mutex;
};
