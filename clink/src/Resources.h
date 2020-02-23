#pragma once

#include "Symbol.h"
#include "WorkQueueStub.h"

// management of resources that are thread-local
class Resources {

 public:
  Resources(SymbolConsumer *consumer, WorkQueue *wq) noexcept
    : consumer(consumer), wq(wq) {
  }

  // It doesn't make sense to copy one of these.
  Resources(const Resources &) = delete;

  Resources(Resources &&other) = default;

  // As above, it doesn't make sense to copy one of these.
  Resources &operator=(const Resources &) = delete;

  Resources &operator=(Resources &&other) = default;

  SymbolConsumer *consumer;
  WorkQueue *wq;
};
