/// does asking “lock free?” about a function crash Clang?

void foo(void);
int x = __atomic_always_lock_free(2, foo);

// XFAIL: version.parse("18.0.0") <= version.parse(os.environ["LLVM_VERSION"]) < version.parse("19.0.0")
// RUN: clink --build-only --database={%t} --debug --parse-c=clang {%s}
