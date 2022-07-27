// RUN: clink --build-only --database {%t} {%s} >/dev/null
// RUN: echo 'select * from symbols;' | sqlite3 {%t}
// CHECK: x|{%s}|0|4|11|
int const x;
