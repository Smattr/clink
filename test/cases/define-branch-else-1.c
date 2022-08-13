/// recognising a macro definition in a disabled #else branch is only possible
/// with newer versions of Libclang

#if 1
#else
#define FOO
#endif

// XFAIL: version.parse(os.environ["LLVM_VERSION"]) < version.parse("10.0.0")
// RUN: clink --build-only --database {%t} --debug {%s} >/dev/null
// RUN: echo 'select * from symbols where name = "FOO";' | sqlite3 {%t}
// CHECK: FOO|{%s}|0|6|9|
