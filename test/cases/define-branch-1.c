/// recognising a macro definition in an enabled #if branch

#if 1
#define FOO
#else
#endif

// RUN: clink --build-only --database={%t} --debug --parse-c=clang {%s} >/dev/null
// RUN: echo 'select * from symbols where name = "FOO";' | sqlite3 {%t}
// CHECK: FOO|{%s}|0|4|9|
