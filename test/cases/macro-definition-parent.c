/// a macro definition should be considered the parent of its contained
/// references

#define FOO ref

#define BAR a_call(x)

#define BAZ /* a comment
               some more lines */ our_ref

// RUN: clink --build-only --database={%t} --parse-c=clang {%s} >/dev/null

// RUN: echo "select symbols.name, records.path, symbols.category, symbols.line, symbols.col, symbols.parent from symbols inner join records on symbols.path = records.id where symbols.name = 'ref';" | sqlite3 {%t}
// CHECK: ref|{%s}|2|4|13|FOO

// RUN: echo "select symbols.name, records.path, symbols.category, symbols.line, symbols.col, symbols.parent from symbols inner join records on symbols.path = records.id where symbols.name = 'a_call';" | sqlite3 {%t}
// CHECK: a_call|{%s}|2|6|13|BAR

// RUN: echo "select symbols.name, records.path, symbols.category, symbols.line, symbols.col, symbols.parent from symbols inner join records on symbols.path = records.id where symbols.name = 'our_ref';" | sqlite3 {%t}
// CHECK: our_ref|{%s}|2|9|35|BAZ
