const int x;

// RUN: clink --build-only --database={%t} --parse-c=clang {%s} >/dev/null
// RUN: echo "select symbols.name, records.path, symbols.category, symbols.line, symbols.col, symbols.parent from symbols inner join records on symbols.path = records.id;" | sqlite3 {%t}
// CHECK: x|{%s}|0|1|11|
