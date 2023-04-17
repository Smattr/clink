/// basic test case of TableGen parsing

def foo : Op<i32> {
  let x = "hello world";
}

// XFAIL: True

// RUN: clink --build-only --database={%t} --debug {%s} >/dev/null

// RUN: echo "select records.path, symbols.line, symbols.col from symbols inner join records on symbols.path = records.id where symbols.name = 'foo';" | sqlite3 {%t}
// CHECK: {%s}|3|5

// we should not have recognised `let` because it is a keyword
// RUN: echo "select records.path, symbols.line, symbols.col from symbols inner join records on symbols.path = records.id where symbols.name = 'let';" | sqlite3 {%t}
// RUN: echo marker
// CHECK: marker

// RUN: echo "select records.path, symbols.line, symbols.col from symbols inner join records on symbols.path = records.id where symbols.name = 'x';" | sqlite3 {%t}
// CHECK: {%s}|4|7
