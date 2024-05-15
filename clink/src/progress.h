/// \file
/// \brief functions for pretty printing progress to the terminal

#pragma once

#include "../../common/compiler.h"
#include <stddef.h>

/** set up terminal for progress output
 *
 * This function needs to be called before any other functions in this header.
 *
 * \param count Total number of items the caller needs to process
 * \return 0 on success or an errno on failure
 */
int progress_init(size_t count);

/** show an informational message on behalf of a thread
 *
 * \param thread_id Thread whose status to update
 * \param fmt A printf-style format string describing the status message
 * \param ... Parameters to accompany the format string
 */
PRINTF(2, 3)
void progress_status(unsigned long thread_id, const char *fmt, ...);

/** show a warning on behalf of a thread
 *
 * \param thread_id Thread that experienced the warning
 * \param fmt A printf-style format string describing the warning message
 * \param ... Parameters to accompany the format string
 */
PRINTF(2, 3) void progress_warn(unsigned long thread_id, const char *fmt, ...);

/** show an error message on behalf of a thread
 *
 * \param thread_id Thread that experienced the error
 * \param fmt A printf-style format string describing the error message
 * \param ... Parameters to accompany the format string
 */
PRINTF(2, 3) void progress_error(unsigned long thread_id, const char *fmt, ...);

/** add one to the progress counter
 *
 * It is assumed that the total number of times this will ever be called is ≤
 * `count` passed in `progress_init`.
 */
void progress_increment(void);

/** indicate progress is complete or has fatally errored
 *
 * After calling this it is invalid to call any other progress function until
 * calling `progress_init` again.
 */
void progress_free(void);
