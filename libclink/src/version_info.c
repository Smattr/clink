#include <clink/version.h>
#include <stdbool.h>

clink_version_info_t clink_version_info(void) {
  clink_version_info_t version = {.version = clink_version()};
#ifndef NDEBUG
  version.with_assertions = true;
#endif
#ifdef __OPTIMIZE__
  version.with_optimisations = true;
#endif
  return version;
}
