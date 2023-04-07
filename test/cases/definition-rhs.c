/// check if multiplication is incorrectly parsed as pointer definition

void foo(void) {
  extern int y;
  extern int z;
  int x = y * z;
}

// RUN: clink --build-only --database={%t} --debug --parse-c=clang {%s} >/dev/null
// RUN: echo "select category from symbols where name = 'z' and line = 6;" | sqlite3 {%t}
// CHECK: 2
