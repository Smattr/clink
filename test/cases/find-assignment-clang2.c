/// can we identify assignments?

int foo(void) {
  int x;
  x = 2;
  return x;
}

// XFAIL: True
// RUN: clink --build-only --database={%t} --debug --parse-c=clang {%s} >/dev/null
// RUN: echo "select symbols.name, records.path, symbols.line, symbols.col from symbols inner join records on symbols.path = records.id where symbols.name = 'x' and symbols.category = 4;" | sqlite3 {%t}
// CHECK: x|{%s}|5|3
