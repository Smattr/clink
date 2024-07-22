/// does the database give us start and end location in addition to pinpoint?

int
  bar;

int foo(void) {
  return 1;
}

// XFAIL: True
// RUN: clink --build-only --database={%t} --debug --parse-c=clang {%s} >/dev/null

// RUN: echo "select start_line from symbols where name = 'bar';" | sqlite3 {%t}
// CHECK: 3

// RUN: echo "select start_col from symbols where name = 'bar';" | sqlite3 {%t}
// CHECK: 1

// RUN: echo "select end_line from symbols where name = 'bar';" | sqlite3 {%t}
// CHECK: 4

// RUN: echo "select end_col from symbols where name = 'bar';" | sqlite3 {%t}
// CHECK: 5

// RUN: echo "select start_line from symbols where name = 'foo';" | sqlite3 {%t}
// CHECK: 6

// RUN: echo "select start_col from symbols where name = 'foo';" | sqlite3 {%t}
// CHECK: 1

// RUN: echo "select end_line from symbols where name = 'foo';" | sqlite3 {%t}
// CHECK: 8

// RUN: echo "select end_col from symbols where name = 'foo';" | sqlite3 {%t}
// CHECK: 1
