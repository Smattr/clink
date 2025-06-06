/// does a NUL byte within the file cause Cscope parsing to infinite loop?
///
/// This was a problem previously observed when asking Cscope to build an
/// uncompressed database. Apparently it was never designed to process NUL
/// bytes.

 #if foo
#define bar
#endif

// RUN: {%timeout} 10s clink --build-only --database={%t} --debug --parse-c=cscope {%s} >/dev/null
