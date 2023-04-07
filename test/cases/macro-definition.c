/// can we recognise macros?

#define FOO /* nothing */
#define BAR something
#define BAZ(x) something_else(x)

// RUN: clink --build-only --database={%t} --parse-c=clang {%s} >/dev/null
// RUN: echo "select * from symbols where category = 0 order by name;" | sqlite3 {%t}
// CHECK: BAR|{%s}|0|4|9|
// CHECK: BAZ|{%s}|0|5|9|
// CHECK: FOO|{%s}|0|3|9|
