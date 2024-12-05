// check we do not recognise things in strings a symbols

void fn() {
  const char *foo = "hello bar";

  const char *baz = "embedded \" and then qux";

  char a = 'b';
}

// RUN: clink --build-only --database={%t}1 --debug --parse-c=clang {%s} >/dev/null

// RUN: echo "select name, line, col from symbols where name = 'foo' and category < 4;" | sqlite3 {%t}1
// CHECK: foo|4|15

// RUN: echo "select name, line, col from symbols where name = 'bar';" | sqlite3 {%t}1
// RUN: echo "marker1"
// CHECK: marker1

// RUN: echo "select name, line, col from symbols where name = 'baz' and category < 4;" | sqlite3 {%t}1
// CHECK: baz|6|15

// RUN: echo "select name, line, col from symbols where name = 'qux';" | sqlite3 {%t}1
// RUN: echo "marker2"
// CHECK: marker2

// RUN: echo "select name, line, col from symbols where name = 'a' and category < 4;" | sqlite3 {%t}1
// CHECK: a|8|8

// RUN: echo "select name, line, col from symbols where name = 'b';" | sqlite3 {%t}1
// RUN: echo "marker3"
// CHECK: marker3

// try this again with the generic parser

// RUN: clink --build-only --database={%t}2 --parse-c=generic --debug {%s} >/dev/null

// RUN: echo "select name, line, col from symbols where name = 'foo';" | sqlite3 {%t}2
// CHECK: foo|4|15

// RUN: echo "select name, line, col from symbols where name = 'bar';" | sqlite3 {%t}2
// RUN: echo "marker4"
// CHECK: marker4

// RUN: echo "select name, line, col from symbols where name = 'baz';" | sqlite3 {%t}2
// CHECK: baz|6|15

// RUN: echo "select name, line, col from symbols where name = 'qux';" | sqlite3 {%t}2
// RUN: echo "marker5"
// CHECK: marker5

// RUN: echo "select name, line, col from symbols where name = 'a';" | sqlite3 {%t}2
// CHECK: a|8|8

// RUN: echo "select name, line, col from symbols where name = 'b';" | sqlite3 {%t}2
// RUN: echo "marker6"
// CHECK: marker6
