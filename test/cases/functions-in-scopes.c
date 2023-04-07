/// check we detect all functions in a particular scenario that was a problem
/// for the Libclang-based parser

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

// RUN: clink --build-only --database={%t} --parse-c=clang {%s} >/dev/null
// RUN: echo "select * from symbols where category = 1 order by name;" | sqlite3 {%t}
// CHECK: f|{%s}|1|11|7|foo
// CHECK: g|{%s}|1|12|5|foo
// CHECK: h|{%s}|1|13|9|foo
// CHECK: i|{%s}|1|14|7|foo
