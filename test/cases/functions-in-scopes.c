/// check we detect all functions in a particular scenario that was a problem
/// for the Libclang-based parser

// RUN: clink --build-only --database {tmp} {__file__} >/dev/null
// RUN: echo 'select * from symbols where category = 1 order by name;' | sqlite3 {tmp}
// CHECK: f|{__file__}|1|18|7|foo
// CHECK: g|{__file__}|1|19|5|foo
// CHECK: h|{__file__}|1|20|9|foo
// CHECK: i|{__file__}|1|21|7|foo

extern int f(void);
void g(void) { }
extern int h(void);
extern void i(void);


void foo(void) {
  if (f()) {
    g();
    if (h()) {
      i();
    }
  }
}
