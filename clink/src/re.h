/// \file
/// \brief regular expression functions

#pragma once

/** check the validity of a regular expression
 *
 * \param pattern Regular expression to check
 * \return 0 if expression could be validated or an errno if not
 */
int re_check(const char *pattern);
