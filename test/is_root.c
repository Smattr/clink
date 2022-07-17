#include "../clink/src/path.h"
#include "test.h"
#include <stddef.h>
#include <unistd.h>

TEST("!is_root(NULL)") { ASSERT(!is_root(NULL)); }

TEST("is_root(\"/\")") { ASSERT(is_root("/")); }
TEST("is_root(\"//\")") { ASSERT(is_root("//")); }
TEST("is_root(\"///\")") { ASSERT(is_root("///")); }

TEST("is_root(\"/./\") (redundant /./)") { ASSERT(is_root("/./")); }
TEST("is_root(\"/././\") (redundant /./)") { ASSERT(is_root("/././")); }

// circular ../
TEST("is_root(\"/../\") (circular ../)") { ASSERT(is_root("/../")); }
TEST("is_root(\"/../../\") (circular ../)") { ASSERT(is_root("/../../")); }

TEST("is_root() with redundant segment") {
  if (access("/usr", R_OK) == 0)
    ASSERT(is_root("/usr/.."));
}

TEST("!is_root(\"/usr\")") {
  if (access("/usr", R_OK) == 0)
    ASSERT(!is_root("/usr"));
}

TEST("!is_root() for non-existent directory") {
  ASSERT(!is_root("/non/existent/directory"));
}
