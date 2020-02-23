#pragma once

#include "CXXParser.h"
#include "Symbol.h"
#include "WorkQueueStub.h"

/* Management of C/C++ and assembly parsers that we'll lazily construct. It's
 * possible we'll not need one or both of them, in which case we don't have to
 * pay the construction overhead.
 *
 * Note that the implicit expectation is that these resources are thread-local.
 * The class assumes it does not have to do any mutual exclusion or
 * synchronisation.
 */
class Resources {

 public:
  Resources(SymbolConsumer *consumer, WorkQueue *wq) noexcept
    : consumer(consumer), wq(wq) {
  }

  // It doesn't make sense to copy one of these.
  Resources(const Resources &) = delete;

  Resources(Resources &&other) noexcept {
    cxx_parser = other.cxx_parser;
    consumer = other.consumer;
    wq = other.wq;
    other.cxx_parser = nullptr;
  }

  ~Resources() {
    delete cxx_parser;
  }

  // As above, it doesn't make sense to copy one of these.
  Resources &operator=(const Resources &) = delete;

  Resources &operator=(Resources &&other) noexcept {
    delete cxx_parser;
    cxx_parser = other.cxx_parser;
    consumer = other.consumer;
    wq = other.wq;
    other.cxx_parser = nullptr;
    return *this;
  }

  CXXParser *get_cxx_parser() {
    if (cxx_parser == nullptr)
      cxx_parser = new CXXParser;
    return cxx_parser;
  }

  SymbolConsumer *consumer;
  WorkQueue *wq;

 private:
  CXXParser *cxx_parser = nullptr;

};
