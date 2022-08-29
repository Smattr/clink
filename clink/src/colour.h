/// \file
/// \brief support for converting coloured text to black-and-white text

#pragma once

#include <stdio.h>

/** output a string, stripping ANSI codes
 *
 * \param s String to output
 * \param stream Stream to print to
 * \return 0 on success
 */
int printf_bw(const char *s, FILE *stream);
