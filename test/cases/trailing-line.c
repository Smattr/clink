/// a trailing blank line should not result in a read out of bounds

// RUN: clink --build-only --database={%t} --debug {%s} >/dev/null

int x;

