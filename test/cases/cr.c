/// CR without LF should count as a line ending

#define FOO // nothingint bar;

#define baz quxint quz;

// RUN: clink --build-only --database={%t} --debug --parse-c=clang {%s} >/dev/null

// RUN: echo "select * from symbols where name = 'FOO';" | sqlite3 {%t}
// CHECK: FOO|{%s}|0|3|9|

// RUN: echo "select * from symbols where name = 'bar';" | sqlite3 {%t}
// CHECK: bar|{%s}|0|4|5|

// RUN: echo "select * from symbols where name = 'baz';" | sqlite3 {%t}
// CHECK: baz|{%s}|0|7|9|

// RUN: echo "select * from symbols where name = 'quz';" | sqlite3 {%t}
// CHECK: quz|{%s}|0|8|5|
