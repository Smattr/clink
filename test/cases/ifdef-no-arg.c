/// check we do not read out of bounds if a preprocessor directive runs right up
/// against the end of the file

// RUN: clink --build-only --database={%t} --debug --parse-c=clang {%s} >/dev/null

#ifdef