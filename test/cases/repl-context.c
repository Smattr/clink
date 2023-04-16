/// can the REPL generate context on demand?

int x;

// parse this file and generate a database
// RUN: clink --build-only --database={%t} --debug --parse-c=clang --syntax-highlighting=lazy {%s} >/dev/null

// At this point we will have no context for this file because it was not syntax
// highlighted. Ask the REPL to give it us anyway.
// RUN: echo "1x" | clink-repl -f {%t}
// CHECK: >> cscope: 1 lines
// CHECK: {%s} x 3 int x;
