/// \file
/// \brief functions for pretty printing progress to the terminal

#pragma once

#include <stddef.h>

/** set up terminal for progress output
 *
 * This function needs to be called before any other functions in this header.
 *
 * \param count Total number of items the caller needs to process
 */
void progress_init(size_t count);

/** show an informational message on behalf of a thread
 *
 * \param thread_id Thread whose status to update
 * \param fmt A printf-style format string describing the status message
 * \param ... Parameters to accompany the format string
 */
__attribute__((format(printf, 2, 3))) void
progress_status(unsigned long thread_id, const char *fmt, ...);

/** show an error message on behalf of a thread
 *
 * \param thread_id Thread that experienced the error
 * \param fmt A printf-style format string describing the error message
 * \param ... Parameters to accompany the format string
 */
__attribute__((format(printf, 2, 3))) void
progress_error(unsigned long thread_id, const char *fmt, ...);

/** add one to the progress counter
 *
 * It is assumed that the total number of times this will ever be called is ≤
 * `count` passed in `progress_init`.
 */
void progress_increment(void);
