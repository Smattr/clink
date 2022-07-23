/// can we recognise macros?

#define FOO /* nothing */
#define BAR something
#define BAZ(x) something_else(x)

// RUN: clink --build-only --database {tmp} {__file__} >/dev/null
// RUN: echo 'select * from symbols where category = 0 order by name;' | sqlite3 {tmp}
// CHECK: BAR|{__file__}|0|4|9|
// CHECK: BAZ|{__file__}|0|5|9|
// CHECK: FOO|{__file__}|0|3|9|
