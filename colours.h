/* Support for converting between different formats for describing the colour
 * and other attributes of terminal text.
 *
 * There are three different formats dealt with below:
 *
 *   1. HTML colours (e.g. "#deadbe")
 *   2. ANSI terminal codes (e.g. "ESC[32;1m")
 *   3. Ncurses colour pairs
 */

#pragma once

#include <string>

/* Turn a six character string representing an HTML color into a value 0-7
 * representing an ANSI terminal code to most closely match that color.
 */
unsigned html_colour_to_ansi(const char *html, size_t length);

// Configure Ncurses colour pairs for use later.
int init_ncurses_colours();

// Equivalent of ncurses' printw, but interpret ANSI colour codes.
void printw_in_colour(const std::string &text);

// Strip ANSI colour codes from a string.
std::string strip_ansi(const std::string &s);
