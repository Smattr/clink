/// can we survive parsing a debug hook designed to crash Clang?

#pragma clang __debug parser_crash

// XFAIL: True
// RUN: clink --build-only --database={%t} --debug --parse-c=clang {%s}
