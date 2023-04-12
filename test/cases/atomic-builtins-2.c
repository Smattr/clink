/// can we recognise calls to the GCC atomic built-ins?

int foo(void) {
  int i = 0;
  return __sync_fetch_and_add(&i, 1);
}

// RUN: clink --build-only --database={%t} --debug --parse-c=clang {%s} >/dev/null
// RUN: echo "select symbols.name, records.path, symbols.category, symbols.line, col from symbols inner join records on symbols.path = records.id where symbols.name = '__sync_fetch_and_add';" | sqlite3 {%t}
// CHECK: __sync_fetch_and_add|{%s}|1|5|10
