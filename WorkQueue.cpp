#include <cstring>
#include <dirent.h>
#include <mutex>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <tuple>
#include <unistd.h>
#include "util.h"
#include "WorkItem.h"
#include "WorkQueue.h"

using namespace std;

WorkQueue::WorkQueue(const string &directory, time_t era_start)
    : era_start(era_start) {
  string prefix = directory + "/";
  DIR *dir = opendir(prefix.c_str());
  if (dir != nullptr)
    directory_stack.push(make_tuple(prefix, dir));
}

bool WorkQueue::push_directory_stack(const string &directory) {
  DIR *dir = opendir(directory.c_str());
  if (dir == nullptr) {
    // Failed to open the new directory. Just discard it.
    return false;
  }

  directory_stack.push(make_tuple(directory, dir));
  return true;
}

WorkItem *WorkQueue::pop() {

  if (!files_to_read.empty()) {
    const string path = files_to_read.front();
    files_to_read.pop();
    WorkItem *wi = new ReadFile(path);
    return wi;
  }

restart1:
  if (directory_stack.empty())
    return nullptr;

restart2:;
  DIR *current;
  string prefix;
  tie(prefix, current) = directory_stack.top();

  for (;;) {
    struct dirent entry, *result;
    if (readdir_r(current, &entry, &result) != 0 || result == nullptr) {
      // Exhausted this directory.
      closedir(current);
      current = nullptr;
      directory_stack.pop();
      goto restart1;
    }

    // If this is a directory, descend into it.
    if (entry.d_type == DT_DIR && strcmp(entry.d_name, ".") &&
          strcmp(entry.d_name, "..")) {
      string dname = prefix + entry.d_name + "/";
      push_directory_stack(dname);
      goto restart2;
    }

    // If this entry is a C/C++ file, see if it is "new".
    if (entry.d_type == DT_REG && (ends_with(entry.d_name, ".c") ||
                                   ends_with(entry.d_name, ".cpp") ||
                                   ends_with(entry.d_name, ".h") ||
                                   ends_with(entry.d_name, ".hpp") ||
                                   ends_with(entry.d_name, ".s") ||
                                   ends_with(entry.d_name, ".S"))) {
      string path = prefix + entry.d_name;
      struct stat buf;
      if (stat(path.c_str(), &buf) < 0 || buf.st_mtime <= era_start) {
        // Consider this file "old".
        continue;
      }

      if (ends_with(entry.d_name, ".c") || ends_with(entry.d_name, ".cpp")
          || ends_with(entry.d_name, ".h") ||
          ends_with(entry.d_name, ".hpp")) {
        return new ParseCXXFile(path);
      } else {
        return new ParseAsmFile(path);
      }
    }

    // If we reached here, the directory entry was irrelevant to us.
  }
}

void WorkQueue::push(const string &path) {
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

WorkItem *ThreadSafeWorkQueue::pop() {
  lock_guard<mutex> guard(stack_mutex);
  return WorkQueue::pop();
}

void ThreadSafeWorkQueue::push(const string &path) {
  lock_guard<mutex> guard(stack_mutex);
  WorkQueue::push(path);
}
