/// can we recognise calls to the GCC built-ins?

void foo(void *dst, const void *src, unsigned len) {
  __builtin_memcpy(dst, src, len);
}

// RUN: clink --build-only --database={%t} --debug --parse-c=clang {%s} >/dev/null
// RUN: echo 'select name, path, category, line, col from symbols where name = "__builtin_memcpy";' | sqlite3 {%t}
// CHECK: __builtin_memcpy|{%s}|1|4|3
