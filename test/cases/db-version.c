/// test that databases from incompatible versions are rejected

int foo(void) {
  return 42;
}

// RUN: clink --build-only --database={%t} --debug --parse-c=clang {%s} >/dev/null
// now modify the database version
// RUN: echo "update metadata set value = 'foo' where key = 'schema_version';" | sqlite3 {%t}
// now the database should be un-openable
// RUN: ! clink --build-only --database={%t} --debug --parse-c=clang {%s} >/dev/null
