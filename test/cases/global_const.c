const int x;

// RUN: clink --build-only --database={%t} --parse-c=clang {%s} >/dev/null
// RUN: echo 'select * from symbols;' | sqlite3 {%t}
// CHECK: x|{%s}|0|1|11|
