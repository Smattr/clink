/// can Cscope-based parsing correctly handle a symlink within a directory?
///
/// Cscope itself cannot handle being passed symlinks. This test case validates
/// that symlinks are resolved before we pass them to Cscope, even when found in
/// a subdirectory of a source path.

// XFAIL: True
// RUN: clink --build-only --database={%t} --debug --parse-c=cscope {%S}/symlink >/dev/null
