/// can we survive parsing a debug hook designed to crash Clang?

#pragma clang __debug parser_crash

// XFAIL: version.parse(os.environ["LLVM_VERSION"]) < version.parse("16.0.0")
// RUN: clink --build-only --database={%t} --debug --parse-c=clang {%s}
