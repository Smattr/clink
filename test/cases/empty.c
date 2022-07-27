/// this is not a case itself, but instructions on how to run against a file in
/// support/

// XFAIL: True
// RUN: clink --build-only --database={%t} --debug {%S}/support/empty.c
