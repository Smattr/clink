/// recognising a macro definition in a disabled #if branch is only possible
/// with newer versions of Libclang

#if 0
#define FOO
#else
#endif

// XFAIL: version.parse(os.environ["LLVM_VERSION"]) < version.parse("10.0.0")
// RUN: clink --build-only --database={%t} --debug --parse-c=clang {%s} >/dev/null
// RUN: echo "select symbols.name, records.path, symbols.category, symbols.line, symbols.col, symbols.parent from symbols inner join records on symbols.path = records.id where symbols.name = 'FOO';" | sqlite3 {%t}
// CHECK: FOO|{%s}|0|5|9|
