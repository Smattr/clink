#pragma once

/// translate a POSIX regex.h error to an errno
__attribute__((visibility("internal")))
int re_err_to_errno(int err);
