/// can we identify assignments?

int foo(void) {
  int x = 0;
  return x;
}

// RUN: clink --build-only --database={%t} --debug --parse-c=cscope {%s} >/dev/null
// RUN: echo "select symbols.name, records.path, symbols.line from symbols inner join records on symbols.path = records.id where symbols.name = 'x' and symbols.category = 4;" | sqlite3 {%t}
// CHECK: x|{%s}|4
