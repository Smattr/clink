/// can Clang parsing handle parameters that are pointers to member functions?

void foo(void (bar::*baz)(void));

// XFAIL: True
// RUN: clink --build-only --database={%t} --debug --parse-cxx=clang {%s} >/dev/null
