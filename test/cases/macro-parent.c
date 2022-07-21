/// check that macro calls come out parented to their containing function

// RUN: clink --build-only --database {tmp} {__file__} >/dev/null
// RUN: echo 'select * from symbols where name = "macro_call" and category = 1;' | sqlite3 {tmp}
// CHECK: macro_call|{__file__}|1|12|3|foo

#define macro_call() printf("hello")
extern void function_call(void);

int foo() {
  function_call();
  macro_call();
  return 0;
}
