/// base test of Yacc/Bison parsing

%require "3.0"

%define parse.error verbose

%code requires {
  #include <stdio.h>
}

%code {
  #include <stdlib.h>

  extern int x;
}

%code top {
  int y;
}

%token A
%token B

%%

s: r {
  z = $1;
};

r: A B {
  x = y;
} | B {
  y = x;
};

%%

void foo(int a, int b) {
  printf("%d %d\n", a, b);
}

// RUN: clink --build-only --database={%t} --debug --parse-yacc=generic {%s} >/dev/null

// RUN: echo "select records.path, symbols.line, symbols.col from symbols inner join records on symbols.path = records.id where symbols.name = 'x';" | sqlite3 {%t}
// CHECK: {%s}|14|14
// CHECK: {%s}|31|3
// CHECK: {%s}|33|7

// RUN: echo "select records.path, symbols.line, symbols.col from symbols inner join records on symbols.path = records.id where symbols.name = 'y';" | sqlite3 {%t}
// CHECK: {%s}|18|7
// CHECK: {%s}|31|7
// CHECK: {%s}|33|3

// RUN: echo "select records.path, symbols.line, symbols.col from symbols inner join records on symbols.path = records.id where symbols.name = 'z';" | sqlite3 {%t}
// CHECK: {%s}|27|3

// RUN: echo "select records.path, symbols.line, symbols.col from symbols inner join records on symbols.path = records.id where symbols.name = 'foo';" | sqlite3 {%t}
// CHECK: {%s}|38|6

// RUN: echo "select records.path, symbols.line, symbols.col from symbols inner join records on symbols.path = records.id where symbols.name = 'a';" | sqlite3 {%t}
// CHECK: {%s}|38|14
// CHECK: {%s}|39|21

// RUN: echo "select records.path, symbols.line, symbols.col from symbols inner join records on symbols.path = records.id where symbols.name = 'b';" | sqlite3 {%t}
// CHECK: {%s}|38|21
// CHECK: {%s}|39|24
