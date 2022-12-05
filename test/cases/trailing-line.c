/// a trailing blank line should not result in a read out of bounds

// RUN: clink --build-only --database={%t} --debug --parse-c=clang {%s} >/dev/null

int x;

