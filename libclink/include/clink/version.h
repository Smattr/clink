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

#ifdef __cplusplus
}
#endif
