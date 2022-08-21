/// check we do not read out of bounds if a preprocessor directive runs right up
/// against the end of the file

// RUN: clink --build-only --database={%t} --debug {%s} >/dev/null

#ifdef