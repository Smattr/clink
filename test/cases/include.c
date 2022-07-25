/// check we can recognise a basic include

#include "foo.h"

// XFAIL: True
// RUN: clink --build-only --database {tmp} --debug {__file__} >/dev/null
// RUN: echo 'select * from symbols;' | sqlite3 {tmp}
// CHECK: foo.h|{__file__}|3|3|11|
