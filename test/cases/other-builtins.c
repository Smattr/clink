/// can we recognise calls to the GCC built-ins?

void foo(void) {
  return;
  __builtin_unreachable();
}

// RUN: clink --build-only --database={%t} --debug --parse-c=clang {%s} >/dev/null
// RUN: echo 'select name, path, category, line, col from symbols where name = "__builtin_unreachable";' | sqlite3 {%t}
// CHECK: __builtin_unreachable|{%s}|1|5|3
