#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/** retrieve version of this library
 *
 * For now, the version of Clink is an opaque string. You cannot use it to
 * determine whether one version is newer than another. The most you can do is
 * `strcmp` two version strings to determine if they are the same.
 *
 * \return A version string
 */
const char *clink_version(void);

#ifdef __cplusplus
}
#endif
