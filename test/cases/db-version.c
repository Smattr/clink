/// test that databases from incompatible versions are rejected

int foo(void) {
  return 42;
}

// RUN: clink --build-only --database={%t} --debug --parse-c=clang {%s} >/dev/null
// now modify the database version
// RUN: echo "pragma user_version = 0xffffffff;" | sqlite3 {%t}
// now the database should be un-openable
// RUN: ! clink --build-only --database={%t} --debug --parse-c=clang {%s} >/dev/null

// the error message should accurately describe the problem
// RUN: ! clink --build-only --database={%t} --parse-c=generic --parse-cxx=generic {%s} 2>&1
// CHECK: {%t} was created by a different, incompatible version of Clink
