#pragma once

#include <dirent.h>
#include <mutex>
#include <queue>
#include <stack>
#include <string>
#include <sys/types.h>
#include "Task.h"
#include <tuple>
#include <unordered_set>
#include "WorkQueueStub.h"

class WorkQueue {

 public:
  WorkQueue(const std::string &directory, time_t era_start_);
  Task *pop();
  void push(const std::string &path);

 private:
  time_t era_start;
  std::stack<std::tuple<std::string, DIR*>> directory_stack;
  std::queue<std::string> files_to_read;
  std::unordered_set<std::string> files_seen;

  bool push_directory_stack(const std::string &directory);

  std::mutex stack_mutex;
};
