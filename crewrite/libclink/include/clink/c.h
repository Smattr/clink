#pragma once

#include <clink/compiler.h>
#include <clink/symbol.h>

// parse a C or C++ source file
int clink_parse_c(
    const char *CLINK_NONNULL filename,
    int (*CLINK_NONNULL callback)(void *CLINK_NULLABLE state,
                                  const clink_symbol_t *CLINK_NONNULL symbol),
    void *CLINK_NULLABLE state);
