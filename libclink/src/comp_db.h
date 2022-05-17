#pragma once

#include <clang-c/CXCompilationDatabase.h>

struct clink_comp_db {

  /// handle to the underlying database
  CXCompilationDatabase db;
};
