#pragma once

#include <clink/clink.h>
#include <string>
#include "WorkQueueStub.h"

class WorkItem {

 public:
  virtual ~WorkItem() {}

  /* Inheritors should implement this method to take whatever action is
   * appropriate to conceptually "do" their chunk of work.
   */
  virtual void run(clink::Database &db, WorkQueue &wq) = 0;

};

class ParseCXXFile : public WorkItem {

 public:
  ParseCXXFile(const std::string &path)
    : path(path) {
  }

  void run(clink::Database &db, WorkQueue &wq) final;

  virtual ~ParseCXXFile() {}

 private:
  std::string path;

};

class ParseAsmFile : public WorkItem {

 public:
  ParseAsmFile(const std::string &path)
    : path(path) {
  }

  void run(clink::Database &db, WorkQueue &wq) final;

 virtual ~ParseAsmFile() {}

 private:
  std::string path;

};

class ReadFile : public WorkItem {

 public:
  ReadFile(const std::string &path)
    : path(path) {
  }

  void run(clink::Database &db, WorkQueue&) final {
    unsigned lineno = 1;
    (void)clink::vim_highlight(path, [&](const std::string &line) {
      (void)db.add(path, lineno, line);
      lineno++;
      return 0;
    });
  }

  virtual ~ReadFile() {}

 private:
  std::string path;

};
