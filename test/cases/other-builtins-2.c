/// can we recognise calls to the GCC built-ins?

void foo(void *dst, const void *src, unsigned len) {
  __builtin_memcpy(dst, src, len);
}

// RUN: clink --build-only --database={%t} --debug --parse-c=clang {%s} >/dev/null
// RUN: echo "select symbols.name, records.path, symbols.category, symbols.line, symbols.col from symbols inner join records on symbols.path = records.id where symbols.name = '__builtin_memcpy';" | sqlite3 {%t}
// CHECK: __builtin_memcpy|{%s}|1|4|3
