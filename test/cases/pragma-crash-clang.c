/// can we survive parsing a debug hook designed to crash Clang?

#pragma clang __debug parser_crash

// RUN: env ASAN_OPTIONS=detect_leaks=0 clink --build-only --database={%t} --debug --parse-c=clang {%s}
