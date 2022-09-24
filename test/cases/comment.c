// check that we do not recognise things in comments as symbols

// foo in a comment

/*
   bar in a multiline comment
 */

// RUN: clink --build-only --database={%t}1 --debug {%s} >/dev/null

// RUN: echo 'select * from symbols where name = "foo";' | sqlite3 {%t}1
// RUN: echo "marker1"
// CHECK: marker1

// RUN: echo 'select * from symbols where name = "bar";' | sqlite3 {%t}1
// RUN: echo "marker2"
// CHECK: marker2

// try this again with the generic parser

// XFAIL: True

// RUN: clink --build-only --database={%t}2 --parse-c=generic --debug {%s} >/dev/null

// RUN: echo 'select * from symbols where name = "foo";' | sqlite3 {%t}2
// RUN: echo "marker3"
// CHECK: marker3

// RUN: echo 'select * from symbols where name = "bar";' | sqlite3 {%t}2
// RUN: echo "marker4"
// CHECK: marker4
