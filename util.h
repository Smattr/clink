#pragma once

#include <cassert>
#include <cstring>
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

static inline bool ends_with(const char *s, const char *suffix) {
    size_t s_len = strlen(s);
    size_t suffix_len = strlen(suffix);
    return s_len >= suffix_len && strcmp(&s[s_len - suffix_len], suffix) == 0;
}
