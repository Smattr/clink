#pragma once

#include <cassert>
#include <cstring>
#include <ctype.h>
#include <string>

static inline std::string lstrip(const std::string &s) {

  std::string t;

  bool dropping = true;
  for (const char &c : s) {
    if (dropping) {
      if (!isspace(c)) {
        t += c;
        dropping = false;
      }
    } else {
      t += c;
    }
  }

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
