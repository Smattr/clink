/// check that database results are left-trimmed before insertion

void foo(void) {
  int x = 0;
}

// parse this into a database with full content extraction
// RUN: clink --build-only --database={%t} --debug --parse-c=clang --syntax-highlight=eager {%s} >/dev/null

// we should have content extracted
// RUN: echo "select COUNT(*) from content where line = 4;" | sqlite3 {%t}
// CHECK: 1

// it should have been stripped for leading space
//   1. ask for the highlighted version of line 4 from this file
//   2. use `sed` to strip ANSI colour sequences
//   3. use `sed` to add a marker to suppress integration.py’s own left-trimming
//      behaviour
// RUN: echo "select body from content where line = 4;" | sqlite3 {%t} | sed -E 's/\x1b\[[0-9;]+m//g' | sed -E 's/(.*)/start\1/'
// CHECK: startint x = 0;
