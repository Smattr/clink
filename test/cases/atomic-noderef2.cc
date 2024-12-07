/// can the Clang parser handle atomic+noderef?
///
/// This used to crash Clang, https://github.com/llvm/llvm-project/issues/116124

namespace {
int *_Atomic x __attribute__((noderef));
}

// RUN: env ASAN_OPTIONS=detect_leaks=0 clink --build-only --database={%t} --debug --parse-cxx=clang {%s}
