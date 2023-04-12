/// recognising a macro definition in an enabled #else branch

#if 0
#else
#define FOO
#endif

// RUN: clink --build-only --database={%t} --debug --parse-c=clang {%s} >/dev/null
// RUN: echo "select symbols.name, records.path, symbols.category, symbols.line, symbols.col, symbols.parent from symbols inner join records on symbols.path = records.id where symbols.name = 'FOO';" | sqlite3 {%t}
// CHECK: FOO|{%s}|0|5|9|
