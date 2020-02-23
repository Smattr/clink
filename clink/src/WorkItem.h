#pragma once

#include "CXXParser.h"
#include "log.h"
#include "Resources.h"
#include <string>
#include "Vim.h"

class WorkItem {

 public:
  virtual ~WorkItem() {}

  /* Inheritors should implement this method to take whatever action is
   * appropriate to conceptually "do" their chunk of work.
   */
  virtual void run(Resources &resources) = 0;

};

class ParseCXXFile : public WorkItem {

 public:
  ParseCXXFile(const std::string &path)
    : path(path) {
  }

  void run(Resources &resources) final {
    resources.consumer->purge(path);
    CXXParser *cxx_parser = resources.get_cxx_parser();
    if (!cxx_parser->load(path.c_str())) {
      // failed
      return;
    }
    cxx_parser->process(*resources.consumer, resources.wq);
    cxx_parser->unload();
  }

  virtual ~ParseCXXFile() {}

 private:
  std::string path;

};

class ParseAsmFile : public WorkItem {

 public:
  ParseAsmFile(const std::string &path)
    : path(path) {
  }

  void run(Resources &resources) final;

 virtual ~ParseAsmFile() {}

 private:
  std::string path;

};

class ReadFile : public WorkItem {

 public:
  ReadFile(const std::string &path)
    : path(path) {
  }

  void run(Resources &resources) final {
    unsigned lineno = 1;
    LOG("highlighting %s", path.c_str());
    for (const std::string &line : vim_highlight(path)) {
      resources.consumer->consume(path, lineno, line);
      lineno++;
    }
  }

  virtual ~ReadFile() {}

 private:
  std::string path;

};
