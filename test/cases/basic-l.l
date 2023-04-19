/// basic test of Lex/Flex parsing

%{
// some prologue
#include <stdio.h>
%}

%option yylineno

%%

foo {
  int x = 1;
  return x;
}

bar {
  int y = 1;
  return y + 1;
}

[A-Z].[0-9] {
  int z = 3;
  return z;
}

// XFAIL: True

// RUN: clink --build-only --database={%t} --debug --parse-lex=generic {%s} >/dev/null

// RUN: echo "select records.path, symbols.line, symbols.col from symbols inner join records on symbols.path = records.id where symbols.name ='x';" | sqlite3 {%t}
// CHECK: {%s}|13|7
// CHECK: {%s}|14|10

// RUN: echo "select records.path, symbols.line, symbols.col from symbols inner join records on symbols.path = records.id where symbols.name ='y';" | sqlite3 {%t}
// CHECK: {%s}|18|7
// CHECK: {%s}|19|10

// RUN: echo "select records.path, symbols.line, symbols.col from symbols inner join records on symbols.path = records.id where symbols.name ='z';" | sqlite3 {%t}
// CHECK: {%s}|23|7
// CHECK: {%s}|24|10
