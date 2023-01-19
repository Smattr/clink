/// \file
/// \brief functions for dealing with the current working directory

#pragma once

/** setup long-lived allocation of the current working directory
 *
 * This function must be called prior to using \p cwd_get.
 *
 * \return 0 on success or an errno on failure
 */
int cwd_init(void);

/** retrieve the current working directory
 *
 * \p cwd_init must be called first.
 *
 * \return The current working directory
 */
const char *cwd_get(void);

/** clean up long-lived allocation of the current working directory
 *
 * This reverses the effect of \p cwd_free. After calling this, it is not valid
 * to call \p cwd_get until calling \p cwd_init again.
 */
void cwd_free(void);
