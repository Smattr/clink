#pragma once

#include "Symbol.h"
#include "WorkQueueStub.h"

class Parser {

 public:
  virtual void process(SymbolConsumer &consumer, WorkQueue *wq) = 0;

  virtual ~Parser() {}

};
