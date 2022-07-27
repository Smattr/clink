/// check if multiplication is incorrectly parsed as pointer definition

void foo(void) {
  int x = y * z;
}

// RUN: clink --build-only --database {%t} --debug {%s} >/dev/null
// RUN: echo 'select * from symbols where name = "z";' | sqlite3 {%t}
// CHECK: z|{%s}|2|4|15|foo
