/// does Cscope-based parsing incorrectly add punctuation to the database?

int main(void) {
  int x[] = {1, 2, 3, 4};
  return 0;
}

// XFAIL: True
// RUN: clink --build-only --database={%t} --debug --parse-c=cscope {%s} >/dev/null
// RUN: echo "select name from symbols where name like '%[]%';" | sqlite3 {%t}
// RUN: echo EOT
// CHECK: EOT
// RUN: echo "select name from symbols where name like '%{{%';" | sqlite3 {%t}
// RUN: echo EOT
// CHECK: EOT
// RUN: echo "select name from symbols where name like '%,%';" | sqlite3 {%t}
// RUN: echo EOT
// CHECK: EOT
// RUN: echo "select name from symbols where name like '%}}%';" | sqlite3 {%t}
// RUN: echo EOT
// CHECK: EOT
