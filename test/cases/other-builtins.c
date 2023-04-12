/// can we recognise calls to the GCC built-ins?

void foo(void) {
  return;
  __builtin_unreachable();
}

// RUN: clink --build-only --database={%t} --debug --parse-c=clang {%s} >/dev/null
// RUN: echo "select symbols.name, records.path, symbols.category, symbols.line, symbols.col from symbols inner join records on symbols.path = records.id where symbols.name = '__builtin_unreachable';" | sqlite3 {%t}
// CHECK: __builtin_unreachable|{%s}|1|5|3
