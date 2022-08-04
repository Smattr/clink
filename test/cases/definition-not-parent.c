/// the LHS of a defining assignment should not be considered the parent of the
/// RHS of the assignment

extern int foo(void);

void bar(void) {
  int r = foo();
}

// RUN: clink --build-only --database {%t} {%s} >/dev/null
// RUN: echo 'select parent from symbols where name = "foo" and category = 1;' | sqlite3 {%t}
// CHECK: bar
