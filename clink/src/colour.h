#pragma once

#include <stdio.h>

/** configure Ncurses colour pairs for use later
 *
 * \returns 0 on success, non-zero on failure
 */
int init_ncurses_colours(void);

/** output a string, stripping ANSI codes
 *
 * \param s String to output
 * \param put putc-style primitive for outputting characters
 * \param stream Stream to print to
 * \returns 0 on success or -1 if put() ever returns EOF
 */
int printf_bw(const char *s, int (*put)(int c, FILE *stream), FILE *stream);
