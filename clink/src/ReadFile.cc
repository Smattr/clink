#include <clink/clink.h>
#include <filesystem>
#include "ReadFile.h"
#include "Task.h"
#include "WorkQueue.h"

ReadFile::ReadFile(const std::filesystem::path &path_): path(path_) { }

void ReadFile::run(clink::Database &db, WorkQueue&) {

  unsigned lineno = 1;
  (void)clink::vim_highlight(path, [&](const std::string &line) {
    (void)db.add(path, lineno, line);
    lineno++;
    return 0;
  });
}
