/// will Cscope-based parsing incorrectly recognise assignments in the non-main
/// file?

// include a secondary file with some complex content
#include <string.h>

int main(void) {
  int y = 42;
  return 0;
}

// RUN: clink --build-only --database={%t} --debug --parse-c=cscope {%s} >/dev/null
// RUN: echo "select name from symbols where name = '__old';" | sqlite3 {%t}
// RUN: echo EOT
// CHECK: EOT
