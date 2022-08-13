/// undefining something should not be counted as a definition

#undef FOO

// RUN: clink --build-only --database {%t} --debug {%s} >/dev/null
// RUN: echo 'select * from symbols;' | sqlite3 {%t}
// CHECK: FOO|{%s}|2|3|8|
