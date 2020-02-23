#include <clink/clink.h>
#include "Options.h"
#include "Resources.h"
#include <string>
#include "Symbol.h"
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

    switch (symbol.category) {

      case clink::Symbol::DEFINITION: {
        SymbolCore s(symbol.name, symbol.path, ST_DEFINITION,
          symbol.lineno, symbol.colno,
          symbol.parent == "" ? nullptr : symbol.parent.c_str());
        resources.consumer->consume(s);
        resources.wq->push(symbol.path);
        break;
      }

      case clink::Symbol::FUNCTION_CALL: {
        SymbolCore s(symbol.name, symbol.path, ST_FUNCTION_CALL,
          symbol.lineno, symbol.colno,
          symbol.parent == "" ? nullptr : symbol.parent.c_str());
        resources.consumer->consume(s);
        resources.wq->push(symbol.path);
        break;
      }

      case clink::Symbol::REFERENCE: {
        SymbolCore s(symbol.name, symbol.path, ST_REFERENCE,
          symbol.lineno, symbol.colno,
          symbol.parent == "" ? nullptr : symbol.parent.c_str());
        resources.consumer->consume(s);
        resources.wq->push(symbol.path);
        break;
      }

      case clink::Symbol::INCLUDE: {
        SymbolCore s(symbol.name, symbol.path, ST_INCLUDE,
          symbol.lineno, symbol.colno,
          symbol.parent == "" ? nullptr : symbol.parent.c_str());
        resources.consumer->consume(s);
        resources.wq->push(symbol.path);
        break;
      }
    }

    return 0;
  });
}

void ParseAsmFile::run(Resources &resources) {

  resources.consumer->purge(path);

  (void)clink::parse_asm(path, [&resources](const clink::Symbol &symbol) {

    switch (symbol.category) {

      case clink::Symbol::DEFINITION: {
        SymbolCore s(symbol.name, symbol.path, ST_DEFINITION,
          symbol.lineno, symbol.colno,
          symbol.parent == "" ? nullptr : symbol.parent.c_str());
        resources.consumer->consume(s);
        resources.wq->push(symbol.path);
        break;
      }

      case clink::Symbol::FUNCTION_CALL: {
        SymbolCore s(symbol.name, symbol.path, ST_FUNCTION_CALL,
          symbol.lineno, symbol.colno,
          symbol.parent == "" ? nullptr : symbol.parent.c_str());
        resources.consumer->consume(s);
        resources.wq->push(symbol.path);
        break;
      }

      case clink::Symbol::INCLUDE: {
        SymbolCore s(symbol.name, symbol.path, ST_INCLUDE,
          symbol.lineno, symbol.colno,
          symbol.parent == "" ? nullptr : symbol.parent.c_str());
        resources.consumer->consume(s);
        resources.wq->push(symbol.path);
        break;
      }

      default:
        __builtin_unreachable();

    }

    return 0;
  });
}
