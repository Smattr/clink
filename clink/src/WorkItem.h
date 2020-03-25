#pragma once

#include <clink/clink.h>
#include "log.h"
#include "Symbol.h"
#include <string>
#include "WorkQueueStub.h"

class WorkItem {

 public:
  virtual ~WorkItem() {}

  /* Inheritors should implement this method to take whatever action is
   * appropriate to conceptually "do" their chunk of work.
   */
  virtual void run(SymbolConsumer &consumer, WorkQueue &wq) = 0;

};

class ParseCXXFile : public WorkItem {

 public:
  ParseCXXFile(const std::string &path)
    : path(path) {
  }

  void run(SymbolConsumer &consumer, WorkQueue &wq) final;

  virtual ~ParseCXXFile() {}

 private:
  std::string path;

};

class ParseAsmFile : public WorkItem {

 public:
  ParseAsmFile(const std::string &path)
    : path(path) {
  }

  void run(SymbolConsumer &consumer, WorkQueue &wq) final;

 virtual ~ParseAsmFile() {}

 private:
  std::string path;

};

class ReadFile : public WorkItem {

 public:
  ReadFile(const std::string &path)
    : path(path) {
  }

  void run(SymbolConsumer &consumer, WorkQueue&) final {
    unsigned lineno = 1;
    LOG("highlighting %s", path.c_str());
    (void)clink::vim_highlight(path, [&](const std::string &line) {
      consumer.consume(path, lineno, line);
      lineno++;
      return 0;
    });
  }

  virtual ~ReadFile() {}

 private:
  std::string path;

};
