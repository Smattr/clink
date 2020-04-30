#pragma once

#include <stdio.h>

/** output a string, stripping ANSI codes
 *
 * \param s String to output
 * \param put putc-style primitive for outputting characters
 * \param stream Stream to print to
 * \returns 0 on success or -1 if put() ever returns EOF
 */
int print_bw(const char *s, int (*put)(int c, FILE *stream), FILE *stream);
