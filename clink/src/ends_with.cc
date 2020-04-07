#include <cstddef>
#include <cstring>
#include "ends_with.h"

bool ends_with(const char *s, const char *suffix) {
  size_t s_len = strlen(s);
  size_t suffix_len = strlen(suffix);
  return s_len >= suffix_len && strcmp(&s[s_len - suffix_len], suffix) == 0;
}
