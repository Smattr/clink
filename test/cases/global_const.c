// RUN: clink --build-only --database {tmp} {__file__} >/dev/null
// RUN: echo 'select * from symbols;' | sqlite3 {tmp}
// CHECK: x|{__file__}|0|4|11|
const int x;
