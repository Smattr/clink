#pragma once

#include <clang-c/Index.h>
#include "Parser.h"
#include <string>
#include "Symbol.h"
#include <vector>
#include "WorkQueueStub.h"

class CXXParser : public Parser {

 public:

  CXXParser();
  virtual ~CXXParser();

  /* Load a new file. Automatically unloads any currently loaded file. Returns
   * true on success.
   */
  bool load(const char *path);

  /* Unload the currently loaded file. This function assumes there is a
   * currently loaded file.
   */
  void unload();

  /* Process the currently loaded file. That is, extract all relevant content.
   * This function assumes there is a currently loaded file.
   */
  void process(SymbolConsumer &consumer, WorkQueue *wq) final;

 private:
  // Internal handle to Clang.
  CXIndex m_index;

  /* The current translation unit. This gets reassigned whenever we load a new
   * file. NULL when no file is currently loaded.
   */
  CXTranslationUnit m_tu = nullptr;

  /* The last file we pushed into the work queue or "" if none. This is used as
   * an optimisation to avoid taking the work queue lock for an operation that
   * we know will be a no-op.
   */
  std::string last_seen;

  friend CXChildVisitResult visitor(CXCursor cursor, CXCursor /* ignored */,
      CXClientData data);
};
