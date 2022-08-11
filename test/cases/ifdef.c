/// a test of some preprocessor features

#ifdef FOO
#define BAR
#endif

#if QUX && QUUX
#elif QUUZ
#endif

// XFAIL: True

// RUN: clink --build-only --database {%t} --debug {%s} >/dev/null

// can we recognise a reference in an ifdef?
// RUN: echo 'select * from symbols where name = "FOO";' | sqlite3 {%t}
// CHECK: FOO|{%s}|2|3|8|

// can we reognise things in #ifs? 
// RUN: echo 'select * from symbols where name = "QUX";' | sqlite3 {%t}
// CHECK: QUX|{%s}|2|7|5|
// RUN: echo 'select * from symbols where name = "QUUX";' | sqlite3 {%t}
// CHECK: QUUX|{%s}|2|7|12|

// can we recognise things in #elifs?
// RUN: echo 'select * from symbols where name = "QUUZ";' | sqlite3 {%t}
// CHECK: QUUZ|{%s}|2|8|7|
