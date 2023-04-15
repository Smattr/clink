/// \file
/// \brief a database without a \p metadata table should be detected as legacy

int x;

// create a database with entries for this file
// RUN: clink --build-only --database={%t} --debug --parse-c=clang {%s} >/dev/null

// delete the metadata table, corresponding to what a pre-versioned Clink
// database would look like
// RUN: echo "drop table metadata;" | sqlite3 {%t}

// now the database should be un-openable
// RUN: ! clink --build-only --database={%t} --debug --parse-c=clang {%s} >/dev/null

// the error message should accurately describe the problem
// RUN: ! clink --build-only --database={%t} --parse-c=generic --parse-cxx=generic {%s} 2>&1
// CHECK: {%t} was created by a different, incompatible version of Clink
