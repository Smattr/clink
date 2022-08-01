/// check we can recognise a basic include

#include "foo.h"

// RUN: clink --build-only --database {%t} --debug {%s} >/dev/null
// RUN: echo 'select * from symbols;' | sqlite3 {%t}
// CHECK: foo.h|{%s}|3|3|1|
