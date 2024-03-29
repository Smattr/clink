/// check we can recognise a basic include

#include "foo.h"

// RUN: clink --build-only --database={%t} --debug --parse-c=clang {%s} >/dev/null
// RUN: echo "select symbols.name, records.path, symbols.category, symbols.line, symbols.col, symbols.parent from symbols inner join records on symbols.path = records.id;" | sqlite3 {%t}
// CHECK: foo.h|{%s}|3|3|1|
