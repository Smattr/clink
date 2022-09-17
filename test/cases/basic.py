// basic Python test

def foo(x: int):
  return x + 1

class Bar():
  def __init__():
    pass

// XFAIL: True

// RUN: clink --build-only --database {%t} --debug {%s} >/dev/null

// RUN: echo 'select path, category, line, col from symbols where name = "foo" and line < 12;' | sqlite3 {%t}
// CHECK: {%s}|0|3|5

// RUN: echo 'select path, line, col from symbols where name = "x" and line < 12 order by line;' | sqlite3 {%t}
// CHECK: {%s}|3|9
// CHECK: {%s}|4|10

// RUN: echo 'select * from symbols where name = "Bar" and line < 12;' | sqlite3 {%t}
// CHECK: Bar|{%s}|0|6|7
