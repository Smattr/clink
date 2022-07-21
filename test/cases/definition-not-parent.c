/// the LHS of a defining assignment should not be considered the parent of the
/// RHS of the assignment

// RUN: clink --build-only --database {tmp} {__file__} >/dev/null
// RUN: echo 'select parent from symbols where name = "foo" and category = 1;' | sqlite3 {tmp}
// CHECK: bar

extern int foo(void);

void bar(void) {
  int r = foo();
}
