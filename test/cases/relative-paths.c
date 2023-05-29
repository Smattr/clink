/// do we correctly store relative paths in the database?

int foo(void) {
  int x;
  return 0;
}

// RUN: cp {%s} {%T}/
// RUN: clink --build-only --database={%t}1 --debug --parse-c=clang {%T}/relative-paths.c >/dev/null
// RUN: echo "select distinct path from records;" | sqlite3 {%t}1
// CHECK: relative-paths.c

// RUN: mkdir {%T}/foo
// RUN: cp {%s} {%T}/foo/
// RUN: clink --build-only --database={%t}2 --debug --parse-c=clang {%T}/foo/relative-paths.c >/dev/null
// RUN: echo "select distinct path from records;" | sqlite3 {%t}2
// CHECK: foo/relative-paths.c
