#pragma once

#include "AsmParser.h"
#include "CXXParser.h"
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
    cxx_parser->process(*resources.consumer);
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

  void run(Resources &resources) final {
    resources.consumer->purge(path);
    AsmParser *asm_parser = resources.get_asm_parser();
    if (!asm_parser->load(path.c_str())) {
      // failed
      return;
    }
    asm_parser->process(*resources.consumer);
    asm_parser->unload();
 }

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
    std::vector<std::string> lines = vim_highlight(path);
    resources.contexts.register_lines(path, lines);
  }

  virtual ~ReadFile() {}

 private:
  std::string path;

};
