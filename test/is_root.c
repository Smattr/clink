#include "../clink/src/path.h"
#include "test.h"
#include <stddef.h>
#include <unistd.h>

TEST("test cases for clink/src/is_root.c:is_root()") {

  // NULL is not the root
  ASSERT(!is_root(NULL));

  // some things that should be root
  ASSERT(is_root("/"));
  ASSERT(is_root("//"));
  ASSERT(is_root("///"));

  // redundant /./ in path
  ASSERT(is_root("/./"));
  ASSERT(is_root("/././"));

  // circular ../
  ASSERT(is_root("/../"));
  ASSERT(is_root("/../../"));

  // redundant segment
  if (access("/usr", R_OK) == 0)
    ASSERT(is_root("/usr/.."));

  // some things that should not be consdered root
  if (access("/usr", R_OK) == 0)
    ASSERT(!is_root("/usr"));
  ASSERT(!is_root("/non/existent/directory"));
}
