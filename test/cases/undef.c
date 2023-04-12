/// undefining something should not be counted as a definition

#undef FOO

// RUN: clink --build-only --database={%t} --debug --parse-c=clang {%s} >/dev/null
// RUN: echo "select symbols.name, records.path, symbols.category, symbols.line, symbols.col, symbols.parent from symbols inner join records on symbols.path = records.id;" | sqlite3 {%t}
// CHECK: FOO|{%s}|2|3|8|
