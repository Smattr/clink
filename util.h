#pragma once

#include <assert.h>
#include <ctype.h>

static inline const char *lstrip(const char *s) {

    if (!s)
        return "\n";

    const char *t = s;
    while (isspace(*t))
        t++;

    if (*t == '\0')
        return "\n";

    return t;
}

#ifdef NDEBUG
    #define unreachable() __builtin_unreachable()
#else
    #define unreachable() assert(!"unreachable")
#endif
