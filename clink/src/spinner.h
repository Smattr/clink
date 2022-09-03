/// \file
/// \brief terminal progress spinner/hourglass

#pragma once

#include <stddef.h>

/** start the spinner spinning
 *
 * This function assumes the caller will not output anything to stdout after
 * calling this and before calling `spinner_off`.
 *
 * \param row Terminal row at which to position the spinner
 * \param column Terminal column at which to position the spinner
 * \return 0 on success or an errno on failure
 */
int spinner_on(size_t row, size_t column);

/** stop the spinner
 *
 * No attempt is made to clear the last state of the spinner from the terminal.
 */
void spinner_off(void);
