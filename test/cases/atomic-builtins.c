/// can we recognise calls to the GCC atomic built-ins?

int foo(void) {
  int i = 0;
  return __atomic_load_n(&i, __ATOMIC_ACQUIRE);
}

// RUN: clink --build-only --database={%t} --debug --parse-c=clang {%s} >/dev/null
// RUN: echo "select symbols.name, records.path, symbols.line, symbols.col from symbols inner join records on symbols.path = records.id where symbols.name = '__ATOMIC_ACQUIRE';" | sqlite3 {%t}
// CHECK: __ATOMIC_ACQUIRE|{%s}|5|30
// RUN: echo "select symbols.name, records.path, symbols.category, symbols.line, symbols.col from symbols inner join records on symbols.path = records.id where symbols.name = '__atomic_load_n';" | sqlite3 {%t}
// CHECK: __atomic_load_n|{%s}|1|5|10
