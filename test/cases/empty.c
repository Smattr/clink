/// this is not a case itself, but instructions on how to run against a file in
/// support/

// RUN: clink --build-only --database={%t} --debug --parse-c=clang {%S}/support/empty.c
