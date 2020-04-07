#include <cstddef>
#include <cstdlib>
#include <ctype.h>
#include <filesystem>
#include <memory>
#include <mutex>
#include "ParseAsm.h"
#include "ParseC.h"
#include "ReadFile.h"
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include "Task.h"
#include <unistd.h>
#include "WorkQueue.h"

WorkQueue::WorkQueue(const std::string &directory, time_t era_start_)
    : era_start(era_start_) {
  push_dir(directory);
}

void WorkQueue::push_dir(const std::filesystem::path &dir) {
  dirs.push(std::make_pair(
    std::filesystem::begin(std::filesystem::directory_iterator(dir)),
    std::filesystem::end(  std::filesystem::directory_iterator(dir))));
}

// case insensitive comparison
static bool eq(const std::string &a, const std::string &b) {

  if (a.size() != b.size())
    return false;

  for (size_t i = 0; i < a.size(); ++i) {
    if (tolower(a[i]) != tolower(b[i]))
      return false;
  }

  return true;
}

static bool is_asm(const std::filesystem::path &p) {
  return eq(p.extension().string(), ".s");
}

static bool is_c(const std::filesystem::path &p) {
  return eq(p.extension().string(), ".c") ||
         eq(p.extension().string(), ".cc") ||
         eq(p.extension().string(), ".cxx") ||
         eq(p.extension().string(), ".cpp") ||
         eq(p.extension().string(), ".h") ||
         eq(p.extension().string(), ".hpp");
}

std::unique_ptr<Task> WorkQueue::pop() {

  std::lock_guard<std::mutex> guard(stack_mutex);

  // do we have a pending file to read?
  if (!files_to_read.empty()) {

    // extract the head of the queue
    std::filesystem::path p = files_to_read.front();
    files_to_read.pop();

    // give the caller a task to read this file
    return std::make_unique<ReadFile>(p);
  }

  while (!dirs.empty()) {

    // if the top of the stack contains an exhausted directory, discard it
    auto &its = dirs.top();
    if (its.first == its.second) {
      dirs.pop();
      continue;
    }

    // read the next entry
    std::filesystem::directory_entry entry = *its.first;
    ++its.first;

    // if it is a directory, descend into it
    if (entry.is_directory()) {
      push_dir(entry.path());
      continue;
    }

    // if this is not an ordinary file, ignore it
    // TODO: should we allow symlinks here?
    if (!entry.is_regular_file())
      continue;

    // if this is not a ASM/C/C++ file, consider it irrelevant
    if (!is_c(entry.path()) && !is_asm(entry.path()))
      continue;

    // check if the file was modified after the last time we saw it
    struct stat buf;
    if (stat(entry.path().string().c_str(), &buf) < 0) {
      // failed; consider it old
      continue;
    }
    if (buf.st_mtime <= era_start) // old
      continue;

    // if it is assembly, pass the caller a task to parse it
    if (is_asm(entry.path()))
      return std::make_unique<ParseAsm>(entry.path().lexically_normal());

    // similarly if it is C/C++
    if (is_c(entry.path()))
      return std::make_unique<ParseC>(entry.path().lexically_normal());
  }

  // if we reached here, there is no further pending work
  return nullptr;
}

void WorkQueue::push(const std::filesystem::path &path) {

  std::lock_guard<std::mutex> guard(stack_mutex);

  // normalise the path
  std::filesystem::path p = path.lexically_normal();

  auto it = files_seen.insert(p.string());
  if (it.second) {
    struct stat buf;
    if (stat(p.string().c_str(), &buf) < 0 || buf.st_mtime <= era_start) {
      // Ignore this file as we already know its contents or can't read it.
      return;
    }
    files_to_read.push(p);
  }
}
