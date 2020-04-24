// test cases for ../../clink/src/is_root.c:is_root()

// force assertions on
#ifdef NDEBUG
  #undef NDEBUG
#endif

#include <assert.h>
#include "../../clink/src/path.h"
#include <stdlib.h>
#include <unistd.h>

int main(void) {

  // NULL is not the root
  assert(!is_root(NULL));

  // some things that should be root
  assert(is_root("/"));
  assert(is_root("//"));
  assert(is_root("///"));

  // redundant /./ in path
  assert(is_root("/./"));
  assert(is_root("/././"));

  // circular ../
  assert(is_root("/../"));
  assert(is_root("/../../"));

  // redundant segment
  if (access("/usr", R_OK) == 0)
    assert(is_root("/usr/.."));

  // some things that should not be consdered root
  if (access("/usr", R_OK) == 0)
    assert(!is_root("/usr"));
  assert(!is_root("/non/existent/directory"));

  return EXIT_SUCCESS;
}
