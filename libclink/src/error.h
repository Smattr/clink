#pragma once

// turn a SQLite error code into a Clink-encoded error number
__attribute__((visibility("internal")))
int sqlite_error(int err);

__attribute__((visibility("internal")))
// turn a POSIX regex error code into a Clink-encoded error number
int regex_error(int err);
