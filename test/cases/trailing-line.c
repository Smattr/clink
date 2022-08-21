/// a trailing blank line should not result in a read out of bounds

// XFAIL: True
// RUN: clink --build-only --database={%t} --debug {%s} >/dev/null

int x;

