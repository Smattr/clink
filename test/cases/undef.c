/// undefining something should not be counted as a definition

#undef FOO

// RUN: clink --build-only --database {tmp} --debug {__file__} >/dev/null
// RUN: echo 'select * from symbols;' | sqlite3 {tmp}
// CHECK: FOO|{__file__}|2|3|8|
