#include <clang-c/Index.h>
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
  version.libclang_major_version = CINDEX_VERSION_MAJOR;
  version.libclang_minor_version = CINDEX_VERSION_MINOR;
  version.llvm_version = LLVM_VERSION;
  return version;
}
