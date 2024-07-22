/// does the database give us exact byte offsets for start and end location?

int
  bar;

// introduce some multi-byte UTF-8 to throw off character-vs-byte count: “”…

int foo(void) {
  return 1;
}

// RUN: clink --build-only --database={%t} --debug --parse-c=clang {%s} >/dev/null

// RUN: echo "select start_byte from symbols where name = 'bar';" | sqlite3 {%t}
// CHECK: 78

// RUN: echo "select end_byte from symbols where name = 'bar';" | sqlite3 {%t}
// CHECK: 86

// RUN: echo "select start_byte from symbols where name = 'foo';" | sqlite3 {%t}
// CHECK: 174

// RUN: echo "select end_byte from symbols where name = 'foo';" | sqlite3 {%t}
// CHECK: 202
