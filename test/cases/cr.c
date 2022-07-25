/// CR without LF should count as a line ending

#define FOO // nothingint bar;

#define baz quxint quz;

// RUN: clink --build-only --database={tmp} --debug {__file__} >/dev/null

// RUN: echo 'select * from symbols where name = "FOO";' | sqlite3 {tmp}
// CHECK: FOO|{__file__}|0|3|9|

// RUN: echo 'select * from symbols where name = "bar";' | sqlite3 {tmp}
// CHECK: bar|{__file__}|0|4|5|

// RUN: echo 'select * from symbols where name = "baz";' | sqlite3 {tmp}
// CHECK: baz|{__file__}|0|7|9|

// RUN: echo 'select * from symbols where name = "quz";' | sqlite3 {tmp}
// CHECK: quz|{__file__}|0|8|5|
