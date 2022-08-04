/// this demonstrates a trick case where multiplication could equally well be a
/// pointer parameter declaration, and there is no way to conclusively tell
/// without context

extern int x;
extern int y;
extern void g(int z);

void foo(void) {
  g(x * y);
}

// RUN: clink --build-only --database {%t} --debug {%s} >/dev/null
// RUN: echo 'select * from symbols where name = "y" and line = 10;' | sqlite3 {%t}
// CHECK: y|{%s}|2|10|9|foo
