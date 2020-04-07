#include <cstddef>
#include <clink/clink.h>
#include <filesystem>
#include "Options.h"
#include "ParseC.h"
#include <string>
#include "Task.h"
#include "WorkQueue.h"
#include <vector>

ParseC::ParseC(const std::filesystem::path &path_): path(path_) { }

void ParseC::run(clink::Database &db, WorkQueue &q) {

  db.remove(path);

  // build up --include args
  std::vector<std::string> includes{options.include_dirs.size() * 2};
  for (size_t i = 0; i < options.include_dirs.size(); ++i) {
    includes[i * 2] = "-I";
    includes[i * 2 + 1] = options.include_dirs[i];
  }

  (void)clink::parse_c(path, includes, [&](const clink::Symbol &symbol) {
    (void)db.add(symbol);
    q.push(symbol.path);
    return 0;
  });
}
