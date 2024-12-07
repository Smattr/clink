/// does asking “lock free?” about a function crash Clang?

void foo(void);
int x = __atomic_always_lock_free(2, foo);

// RUN: env ASAN_OPTIONS=detect_leaks=0 clink --build-only --database={%t} --debug --parse-c=clang {%s}
