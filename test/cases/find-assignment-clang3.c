/// can we identify assignments?

int foo(void) {
  static int x;
  ++x;
  return x;
}

// XFAIL: True
// RUN: clink --build-only --database={%t} --debug --parse-c=clang {%s} >/dev/null
// RUN: echo "select symbols.name, records.path, symbols.line from symbols inner join records on symbols.path = records.id where symbols.name = 'x' and symbols.category = 4;" | sqlite3 {%t}
// CHECK: x|{%s}|5
