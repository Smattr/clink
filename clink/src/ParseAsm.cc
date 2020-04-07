#include <cstddef>
#include <clink/clink.h>
#include <filesystem>
#include "ParseAsm.h"
#include "Task.h"
#include "WorkQueue.h"

ParseAsm::ParseAsm(const std::filesystem::path &path_): path(path_) { }

void ParseAsm::run(clink::Database &db, WorkQueue &q) {

  db.remove(path);

  (void)clink::parse_asm(path, [&](const clink::Symbol &symbol) {
    (void)db.add(symbol);
    q.push(symbol.path);
    return 0;
  });
}

