/// check we can recognise a basic include

// XFAIL: True
// RUN: clink --build-only --database {tmp} {__file__} >/dev/null
// RUN: echo 'select * from symbols;' | sqlite3 {tmp}
// CHECK: foo.h|{__file__}|3|8|1|

#include "foo.h"
