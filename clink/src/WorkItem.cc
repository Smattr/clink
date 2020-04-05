#include <clink/clink.h>
#include "Options.h"
#include <string>
#include <vector>
#include "WorkItem.h"
#include "WorkQueue.h"

void ParseCXXFile::run(clink::Database &db, WorkQueue &wq) {

  db.remove(path);

  // build up --include args
  std::vector<std::string> includes{options.include_dirs.size() * 2};
  for (size_t i = 0; i < options.include_dirs.size(); ++i) {
    includes[i * 2] = "-I";
    includes[i * 2 + 1] = options.include_dirs[i];
  }

  (void)clink::parse_c(path, includes, [&](const clink::Symbol &symbol) {
    (void)db.add(symbol);
    wq.push(symbol.path);
    return 0;
  });
}

void ParseAsmFile::run(clink::Database &db, WorkQueue &wq) {

  db.remove(path);

  (void)clink::parse_asm(path, [&](const clink::Symbol &symbol) {
    (void)db.add(symbol);
    wq.push(symbol.path);
    return 0;
  });
}
