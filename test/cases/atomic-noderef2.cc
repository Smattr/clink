/// can the Clang parser handle atomic+noderef?
///
/// This used to crash Clang, https://github.com/llvm/llvm-project/issues/116124

namespace {
int *_Atomic x __attribute__((noderef));
}

// XFAIL: version.parse(os.environ["LLVM_VERSION"]) < version.parse("20.0.0")
// RUN: clink --build-only --database={%t} --debug --parse-cxx=clang {%s}
