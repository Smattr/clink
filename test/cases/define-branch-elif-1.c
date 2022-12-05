/// recognising a macro definition in an enabled #elif branch

#if 0
#elif 1
#define FOO
#endif

// RUN: clink --build-only --database={%t} --debug --parse-c=clang {%s} >/dev/null
// RUN: echo 'select * from symbols where name = "FOO";' | sqlite3 {%t}
// CHECK: FOO|{%s}|0|5|9|
