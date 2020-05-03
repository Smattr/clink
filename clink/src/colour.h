// Support for converting between different formats for describing the colour
// and other attributes of terminal text.
//
// There are two different formats dealt with below:
//
//   1. ANSI terminal codes (e.g. "ESC[32;1m")
//   2. Ncurses colour pairs

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
 * \param stream Stream to print to
 * \returns 0 on success
 */
int printf_bw(const char *s, FILE *stream);

/** output a string using Ncurses, stripping ANSI codes
 *
 * \param s String to output
 * \returns 0 on success
 */
void printw_bw(const char *s);

/** output a string using Ncurses, translating ANSI codes to Ncurses colours
 *
 * \param s String to output
 * \returns 0 on success
 */
void printw_colour(const char *s);
