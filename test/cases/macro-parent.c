/// check that macro calls come out parented to their containing function

#define macro_call() printf("hello")
extern void function_call(void);

int foo() {
  function_call();
  macro_call();
  return 0;
}

// RUN: clink --build-only --database={%t} --parse-c=clang {%s} >/dev/null
// RUN: echo "select symbols.name, records.path, symbols.category, symbols.line, symbols.col, symbols.parent from symbols inner join records on symbols.path = records.id where symbols.name = 'macro_call' and symbols.category = 1;" | sqlite3 {%t}
// CHECK: macro_call|{%s}|1|8|3|foo
