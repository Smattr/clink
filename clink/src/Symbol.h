#pragma once

#include <clink/clink.h>
#include <string>

class SymbolConsumer {

 public:
  virtual void consume(const clink::Symbol &s) = 0;
  virtual void consume(const std::string &path, unsigned lineno,
    const std::string &line) = 0;
  virtual bool purge(const std::string &path) = 0;

  virtual ~SymbolConsumer() {}

};
