#include <clink/clink.h>
#include "Resources.h"
#include "Symbol.h"
#include "WorkItem.h"
#include "WorkQueue.h"

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
