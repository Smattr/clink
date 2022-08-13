#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CLINK_API
#define CLINK_API __attribute__((visibility("default")))
#endif

/** retrieve version of this library
 *
 * For now, the version of Clink is an opaque string. You cannot use it to
 * determine whether one version is newer than another. The most you can do is
 * `strcmp` two version strings to determine if they are the same.
 *
 * \return A version string
 */
CLINK_API const char *clink_version(void);

/// return value of `clink_version_info`
typedef struct {
  const char *version;             ///< version name (return of `clink_version`)
  unsigned with_assertions : 1;    ///< was the library built with runtime
                                   ///< assertions enabled?
  unsigned with_optimisations : 1; ///< was -O1 or higher compiler optimisation
                                   ///< used?
  unsigned libclang_major_version; ///< major version number of libclang we were
                                   ///< built against
  unsigned libclang_minor_version; ///< minor version number of libclang we were
                                   ///< built against
  const char *llvm_version; ///< version of libclang-containing LLVM we were
                            ///< built against
} clink_version_info_t;

/** return extended version information of this library
 *
 * \return version information, see `clink_version_info_t`
 */
CLINK_API clink_version_info_t clink_version_info(void);

#ifdef __cplusplus
}
#endif
