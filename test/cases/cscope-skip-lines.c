/// does Cscope-based parsing incorrectly miss symbols?

extern void foo(float a, float b, float c);

int main(void) {
  int x = 42;
  return 0;
}

// RUN: clink --build-only --database={%t} --debug --parse-c=cscope {%s} >/dev/null
// RUN: echo "select name from symbols where name = 'a';" | sqlite3 {%t}
// CHECK: a
// RUN: echo "select name from symbols where name = 'b';" | sqlite3 {%t}
// CHECK: b
// RUN: echo "select name from symbols where name = 'c';" | sqlite3 {%t}
// CHECK: c
