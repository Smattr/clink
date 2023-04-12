/// a test of some preprocessor features

#ifdef FOO
#define BAR
#endif

#if QUX && QUUX
#elif QUUZ
#endif

// RUN: clink --build-only --database={%t} --debug --parse-c=clang {%s} >/dev/null

// can we recognise a reference in an ifdef?
// RUN: echo "select symbols.name, records.path, symbols.category, symbols.line, symbols.col, symbols.parent from symbols inner join records on symbols.path = records.id where symbols.name = 'FOO';" | sqlite3 {%t}
// CHECK: FOO|{%s}|2|3|8|

// can we reognise things in #ifs? 
// RUN: echo "select symbols.name, records.path, symbols.category, symbols.line, symbols.col, symbols.parent from symbols inner join records on symbols.path = records.id where symbols.name = 'QUX';" | sqlite3 {%t}
// CHECK: QUX|{%s}|2|7|5|
// RUN: echo "select symbols.name, records.path, symbols.category, symbols.line, symbols.col, symbols.parent from symbols inner join records on symbols.path = records.id where symbols.name = 'QUUX';" | sqlite3 {%t}
// CHECK: QUUX|{%s}|2|7|12|

// can we recognise things in #elifs?
// RUN: echo "select symbols.name, records.path, symbols.category, symbols.line, symbols.col, symbols.parent from symbols inner join records on symbols.path = records.id where symbols.name = 'QUUZ';" | sqlite3 {%t}
// CHECK: QUUZ|{%s}|2|8|7|
