#pragma once

#include <clink/symbol.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// parse a C/C++ file
//
// Returns 0 on success. If your callback ever returns non-zero, parsing will be
// terminated and this value will be returned.
int clink_parse_c(
    const char *filename,
    const char **clang_argv,
    size_t clang_argc,
    int (*callback)(const struct clink_symbol *symbol, void *state),
    void *state);

#ifdef __cplusplus
}
#endif
