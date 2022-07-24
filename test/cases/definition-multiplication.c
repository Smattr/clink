/// this demonstrates a trick case where multiplication could equally well be a
/// pointer parameter declaration, and there is no way to conclusively tell
/// without context

void foo(void) {
  g(x * y);
}

// XFAIL: True

// RUN: clink --build-only --database {tmp} --debug {__file__} >/dev/null
// RUN: echo 'select * from symbols where name = "y";' | sqlite3 {tmp}
// CHECK: y|{__file__}|2|6|9|foo
