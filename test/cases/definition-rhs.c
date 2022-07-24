/// check if multiplication is incorrectly parsed as pointer definition

void foo(void) {
  int x = y * z;
}

// RUN: clink --build-only --database {tmp} --debug {__file__} >/dev/null
// RUN: echo 'select * from symbols where name = "z";' | sqlite3 {tmp}
// CHECK: z|{__file__}|2|4|15|foo
