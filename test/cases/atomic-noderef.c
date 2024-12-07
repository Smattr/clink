/// can the Clang parser handle atomic+noderef?
///
/// This used to crash Clang, https://github.com/llvm/llvm-project/issues/116124

int *_Atomic x __attribute__((noderef));

// XFAIL: is_using_asan(shutil.which("clink")) and version.parse("18.0.0") <= version.parse(os.environ["LLVM_VERSION"]) < version.parse("19.0.0")
// RUN: {%timeout} 5s env ASAN_OPTIONS=detect_leaks=0 clink --build-only --database={%t} --debug --parse-c=clang {%s}
