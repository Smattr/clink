/// this demonstrates a trick case where multiplication could equally well be a
/// pointer parameter declaration, and there is no way to conclusively tell
/// without context

void foo(void) {
  g(x * y);
}

// XFAIL: True

// RUN: clink --build-only --database {%t} --debug {%s} >/dev/null
// RUN: echo 'select * from symbols where name = "y";' | sqlite3 {%t}
// CHECK: y|{%s}|2|6|9|foo
