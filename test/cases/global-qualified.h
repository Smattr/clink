/// does static cast of a globally qualified thing cause an infinite loop?

void baz() { static_cast<foo>(::bar); }

// XFAIL: version.parse(os.environ["LLVM_VERSION"]) >= version.parse("16.0.0")
// RUN: {%timeout} 5s env ASAN_OPTIONS=detect_leaks=0 clink --build-only --database={%t} --debug --parse-c=clang --parse-cxx=clang {%s}
