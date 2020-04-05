#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <memory>
#include <mutex>
#include "ParseAsm.h"
#include "ParseC.h"
#include "ReadFile.h"
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include "Task.h"
#include <tuple>
#include <unistd.h>
#include "util.h"
#include "WorkQueue.h"

WorkQueue::WorkQueue(const std::string &directory, time_t era_start_)
    : era_start(era_start_) {
  std::string prefix = directory + "/";
  DIR *dir = opendir(prefix.c_str());
  if (dir != nullptr)
    directory_stack.push(make_tuple(prefix, dir));
}

bool WorkQueue::push_directory_stack(const std::string &directory) {
  DIR *dir = opendir(directory.c_str());
  if (dir == nullptr) {
    // Failed to open the new directory. Just discard it.
    return false;
  }

  directory_stack.push(make_tuple(directory, dir));
  return true;
}

static std::string normalise_path(const std::string &path) {
  char resolved[PATH_MAX];

  if (realpath(path.c_str(), resolved) == nullptr) {
    // failed
    return path;
  }

  // Try to turn the absolute path into a relative one.
  char cwd[PATH_MAX];
  if (getcwd(cwd, sizeof(cwd)) == nullptr) {
    // failed
    return path;
  }

  const char *relative;
  if (strncmp(resolved, cwd, strlen(cwd)) == 0 &&
      resolved[strlen(cwd)] == '/') {
    relative = &resolved[strlen(cwd) + 1];
  } else {
    relative = resolved;
  }

  return relative;
}

std::unique_ptr<Task> WorkQueue::pop() {

  std::lock_guard<std::mutex> guard(stack_mutex);

  if (!files_to_read.empty()) {
    const std::string path = normalise_path(files_to_read.front());;
    files_to_read.pop();
    return std::make_unique<ReadFile>(path);
  }

restart1:
  if (directory_stack.empty())
    return nullptr;

restart2:;
  DIR *current;
  std::string prefix;
  std::tie(prefix, current) = directory_stack.top();

  for (;;) {
    struct dirent *entry = readdir(current);
    if (entry == nullptr) {
      // Exhausted this directory.
      closedir(current);
      current = nullptr;
      directory_stack.pop();
      goto restart1;
    }

    // If this is a directory, descend into it.
    if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") &&
          strcmp(entry->d_name, "..")) {
      std::string dname = prefix + entry->d_name + "/";
      push_directory_stack(dname);
      goto restart2;
    }

    // If this entry is a C/C++ file, see if it is "new".
    if (entry->d_type == DT_REG && (ends_with(entry->d_name, ".c") ||
                                   ends_with(entry->d_name, ".cpp") ||
                                   ends_with(entry->d_name, ".h") ||
                                   ends_with(entry->d_name, ".hpp") ||
                                   ends_with(entry->d_name, ".s") ||
                                   ends_with(entry->d_name, ".S"))) {
      std::string path = prefix + entry->d_name;
      struct stat buf;
      if (stat(path.c_str(), &buf) < 0 || buf.st_mtime <= era_start) {
        // Consider this file "old".
        continue;
      }

      if (ends_with(entry->d_name, ".c") || ends_with(entry->d_name, ".cpp")
          || ends_with(entry->d_name, ".h") ||
          ends_with(entry->d_name, ".hpp")) {
        return std::make_unique<ParseC>(normalise_path(path));
      } else {
        return std::make_unique<ParseAsm>(normalise_path(path));
      }
    }

    // If we reached here, the directory entry was irrelevant to us.
  }
}

void WorkQueue::push(const std::string &path) {

  std::lock_guard<std::mutex> guard(stack_mutex);

  auto it = files_seen.insert(path);
  if (it.second) {
    struct stat buf;
    if (stat(path.c_str(), &buf) < 0 || buf.st_mtime <= era_start) {
      // Ignore this file as we already know its contents or can't read it.
      return;
    }
    files_to_read.push(path);
  }
}
