/// symbols without parents should be listed with themselves as a parent in the
/// REPL
/// (no, this does not make any sense, but it is what Cscope does)

void foo() {
}

// RUN: clink --build-only --database={%t} --debug --parse-c=clang {%s} >/dev/null
// RUN: echo '0foo' | clink-repl -dl -f {%t}
// CHECK: >> cscope: 1 lines
// CHECK: {%s} foo 5 void foo() {{
