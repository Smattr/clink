#pragma once

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CLINK_API
#define CLINK_API __attribute__((visibility("default")))
#endif

/** set destination for debug messages
 *
 * On startup, debug messages are suppressed. This function (or one of its
 * alternatives below) must be called to enable debugging.
 *
 * \param The stream to write debug messages to or `NULL` to suppress debugging
 *   output
 * \returns The previous stream set for debug messages
 */
CLINK_API FILE *clink_set_debug(FILE *stream);

/** enable debug messages to stderr
 *
 * This is a shorthand for `(void)clink_set_debug(stderr)`.
 */
CLINK_API void clink_debug_on(void);

/** disable debug messages
 *
 * This is a shorthand for `(void)clink_set_debug(NULL)`.
 */
CLINK_API void clink_debug_off(void);

#ifdef __cplusplus
}
#endif
