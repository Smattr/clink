#pragma once

// turn a SQLite error code into a Clink-encoded error number
__attribute__((visibility("internal")))
int sqlite_error(int err);

// turn a libclang error code into a Clink-encoded error number
__attribute__((visibility("internal")))
int clang_error(int err);

// turn a POSIX regex error code into a Clink-encoded error number
__attribute__((visibility("internal")))
int regex_error(int err);
