#include <clink/clink.h>
#include "Options.h"
#include "Resources.h"
#include <string>
#include <vector>
#include "WorkItem.h"
#include "WorkQueue.h"

void ParseCXXFile::run(Resources &resources) {

  resources.consumer->purge(path);

  // build up --include args
  std::vector<std::string> includes{opts.include_dirs.size() * 2};
  for (size_t i = 0; i < opts.include_dirs.size(); ++i) {
    includes[i * 2] = "-I";
    includes[i * 2 + 1] = opts.include_dirs[i];
  }

  (void)clink::parse_c(path, includes, [&resources](const clink::Symbol &symbol) {
    resources.consumer->consume(symbol);
    resources.wq->push(symbol.path);
    return 0;
  });
}

void ParseAsmFile::run(Resources &resources) {

  resources.consumer->purge(path);

  (void)clink::parse_asm(path, [&resources](const clink::Symbol &symbol) {
    resources.consumer->consume(symbol);
    resources.wq->push(symbol.path);
    return 0;
  });
}
